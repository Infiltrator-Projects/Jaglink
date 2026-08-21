// SPDX-License-Identifier: GPL-3.0-or-later
#import "JagLinkDiagnosticsController.h"

#import "MBLinkBLETransport+MBLINK.h"
#import "mblink/elm327.h"
#import "mblink/elm327_session.h"
#import "mblink/obd2.h"
#import "mblink/scheduler.h"
#import "mblink/telemetry.h"

#include <stdint.h>

typedef NS_ENUM(NSInteger, JagLinkDiagnosticsPhase) {
    JagLinkDiagnosticsPhaseIdle = 0,
    JagLinkDiagnosticsPhaseInitializing,
    JagLinkDiagnosticsPhaseCheckingPids,
    JagLinkDiagnosticsPhaseScanningStoredDTCs,
    JagLinkDiagnosticsPhaseScanningPendingDTCs,
    JagLinkDiagnosticsPhaseScanningPermanentDTCs,
    JagLinkDiagnosticsPhaseReadingLive,
    JagLinkDiagnosticsPhaseLive,
    JagLinkDiagnosticsPhaseFailed
};

@interface JagLinkDiagnosticsController () <MBLinkBLETransportDelegate>
@property(nonatomic, copy, readwrite) NSString *statusText;
@property(nonatomic, copy, readwrite, nullable) NSString *peripheralName;
@property(nonatomic, copy, readwrite, nullable) NSString *adapterIdentifier;
@property(nonatomic, copy, readwrite) NSString *faultScanStatusText;
@property(nonatomic, copy, readwrite) NSArray<NSString *> *storedDTCs;
@property(nonatomic, copy, readwrite) NSArray<NSString *> *pendingDTCs;
@property(nonatomic, copy, readwrite) NSArray<NSString *> *permanentDTCs;
@property(nonatomic, readwrite, getter=isActive) BOOL active;
@property(nonatomic, readwrite, getter=isReady) BOOL ready;

- (void)handleSessionEvent:(const MblinkElm327Session *)session;
- (void)processCompletedResponse;
- (void)beginPortableSession;
- (void)beginCurrentInitializationCommand;
- (BOOL)beginCommand:(const char *)command timeout:(uint64_t)timeoutMs;
- (void)startTickTimer;
- (void)stopTickTimer;
- (void)processInitializationResponse:(const MblinkElm327Response *)response;
- (void)processSupportedPidResponse:(const MblinkElm327Response *)response;
- (void)beginFaultScan;
- (void)beginFaultScanKind:(MblinkObd2DtcKind)kind;
- (void)processFaultScanResponse:(const MblinkElm327Response *)response;
- (void)completeLiveSetup;
- (void)processLiveResponse:(const MblinkElm327Response *)response;
- (void)scheduleNextLiveRequest;
@end

@implementation JagLinkDiagnosticsController {
    MBLinkBLETransport *_provider;
    MblinkElm327Session _session;
    BOOL _sessionInitialized;
    MblinkElm327InitState _initialization;
    MblinkObd2PidSet _supportedPids;
    MblinkScheduler _scheduler;
    MblinkTelemetryStore _telemetry;
    MblinkTelemetryRecorder _recorder;
    MblinkTelemetrySessionMetadata _sessionMetadata;
    NSMutableData *_sessionCSV;
    JagLinkDiagnosticsPhase _phase;
    dispatch_source_t _tickTimer;
    size_t _activeScheduleIndex;
    uint8_t _activePid;
    uint8_t _supportedPidBase;
    NSUInteger _pollGeneration;
    uint64_t _sessionMonotonicStartMs;
}

static uint64_t JagLinkMonotonicMilliseconds(void)
{
    NSTimeInterval uptime = NSProcessInfo.processInfo.systemUptime;
    if (uptime <= 0.0) return 0U;
    const double milliseconds = uptime * 1000.0;
    return milliseconds >= (double)UINT64_MAX ? UINT64_MAX : (uint64_t)milliseconds;
}

static uint64_t JagLinkElapsedMilliseconds(uint64_t startedMs)
{
    const uint64_t nowMs = JagLinkMonotonicMilliseconds();
    return nowMs >= startedMs ? nowMs - startedMs : 0U;
}

static uint64_t JagLinkEpochMilliseconds(void)
{
    NSTimeInterval seconds = [NSDate date].timeIntervalSince1970;
    if (seconds <= 0.0) return 0U;
    const double milliseconds = seconds * 1000.0;
    return milliseconds >= (double)UINT64_MAX ? UINT64_MAX : (uint64_t)milliseconds;
}

static bool JagLinkAppendCSV(void *context, const char *bytes, size_t length)
{
    if (context == NULL || bytes == NULL) return false;
    NSMutableData *data = (__bridge NSMutableData *)context;
    [data appendBytes:bytes length:length];
    return true;
}

static void JagLinkSessionEvent(void *context, const MblinkElm327Session *session)
{
    JagLinkDiagnosticsController *controller = (__bridge JagLinkDiagnosticsController *)context;
    if (controller != nil && session != NULL) [controller handleSessionEvent:session];
}

static NSString *JagLinkStringFromCString(const char *value)
{
    if (value == NULL) return @"unknown";
    NSString *string = [NSString stringWithUTF8String:value];
    return string != nil ? string : @"unknown";
}

static NSArray<NSString *> *JagLinkDTCStrings(const MblinkObd2DtcList *list)
{
    if (list == NULL || list->count == 0U) return @[];
    NSMutableArray<NSString *> *values = [[NSMutableArray alloc] initWithCapacity:list->count];
    for (size_t index = 0U; index < list->count; ++index) {
        NSString *code = JagLinkStringFromCString(list->entries[index].code);
        if (code.length != 0U) [values addObject:code];
    }
    return [values copy];
}

- (instancetype)init
{
    self = [super init];
    if (self != nil) {
        _provider = [[MBLinkBLETransport alloc] init];
        _provider.delegate = self;
        _statusText = @"Idle";
        _faultScanStatusText = @"Not scanned";
        _storedDTCs = @[];
        _pendingDTCs = @[];
        _permanentDTCs = @[];
        _phase = JagLinkDiagnosticsPhaseIdle;
        mblink_obd2_pid_set_clear(&_supportedPids);
        mblink_scheduler_init(&_scheduler);
        mblink_telemetry_store_init(&_telemetry);
        mblink_telemetry_recorder_init(&_recorder);
        _sessionCSV = [[NSMutableData alloc] init];
        mblink_telemetry_store_set_favourite(&_telemetry, 0x0cU, true);
        mblink_telemetry_store_set_favourite(&_telemetry, 0x0dU, true);
        mblink_telemetry_store_set_favourite(&_telemetry, 0x05U, true);
        mblink_telemetry_store_set_favourite(&_telemetry, 0x0bU, true);
        mblink_telemetry_session_metadata_init(&_sessionMetadata, 0U, NULL, NULL);
    }
    return self;
}

- (void)dealloc
{
    _provider.delegate = nil;
    [self stopTickTimer];
    if (_recorder.started && !_recorder.finished) (void)mblink_telemetry_recorder_finish(&_recorder, JagLinkEpochMilliseconds());
    if (_sessionInitialized) {
        _sessionInitialized = NO;
        mblink_elm327_session_disconnect(&_session);
        mblink_elm327_session_deinit(&_session);
    } else {
        [_provider disconnect];
    }
}

- (void)notifyDelegate
{
    id<JagLinkDiagnosticsControllerDelegate> delegate = self.delegate;
    if (delegate != nil) [delegate diagnosticsControllerDidUpdate:self];
}

- (void)setStatus:(NSString *)status
{
    self.statusText = status;
    [self notifyDelegate];
}

- (void)start
{
    if (![NSThread isMainThread]) { dispatch_async(dispatch_get_main_queue(), ^{ [self start]; }); return; }
    if (self.active) return;

    _pollGeneration++;
    self.active = YES;
    self.ready = NO;
    self.faultScanStatusText = @"Waiting for vehicle connection";
    self.storedDTCs = @[];
    self.pendingDTCs = @[];
    self.permanentDTCs = @[];
    _phase = JagLinkDiagnosticsPhaseIdle;
    _activePid = 0U;
    _activeScheduleIndex = 0U;
    _supportedPidBase = 0U;
    mblink_obd2_pid_set_clear(&_supportedPids);
    mblink_scheduler_init(&_scheduler);
    mblink_telemetry_store_clear_samples(&_telemetry);
    mblink_telemetry_recorder_init(&_recorder);
    _sessionCSV = [[NSMutableData alloc] init];
    _sessionMonotonicStartMs = JagLinkMonotonicMilliseconds();
    mblink_telemetry_session_metadata_init(&_sessionMetadata, JagLinkEpochMilliseconds(), NULL, NULL);
    [self notifyDelegate];
    [_provider start];
}

- (void)disconnect
{
    if (![NSThread isMainThread]) { dispatch_async(dispatch_get_main_queue(), ^{ [self disconnect]; }); return; }
    _pollGeneration++;
    [self stopTickTimer];
    if (_sessionInitialized) {
        _sessionInitialized = NO;
        mblink_elm327_session_disconnect(&_session);
        mblink_elm327_session_deinit(&_session);
    } else {
        [_provider disconnect];
    }
    const uint64_t endedEpochMs = JagLinkEpochMilliseconds();
    mblink_telemetry_session_metadata_finish(&_sessionMetadata, endedEpochMs);
    if (_recorder.started && !_recorder.finished) (void)mblink_telemetry_recorder_finish(&_recorder, endedEpochMs);
    _phase = JagLinkDiagnosticsPhaseIdle;
    self.active = NO;
    self.ready = NO;
    [self setStatus:@"Disconnected"];
}

- (void)bleTransportDidUpdate:(MBLinkBLETransport *)transport
{
    self.peripheralName = transport.peripheralName;
    self.adapterIdentifier = transport.adapterIdentifier;
    if (transport.adapterIdentifier != nil) mblink_telemetry_session_metadata_set_adapter(&_sessionMetadata, transport.adapterIdentifier.UTF8String);

    if (transport.isReady && !_sessionInitialized) { [self beginPortableSession]; return; }
    if (!transport.isReady && _sessionInitialized && transport.state != MBLinkBLETransportStateProbing) {
        _pollGeneration++;
        [self stopTickTimer];
        _sessionInitialized = NO;
        mblink_elm327_session_deinit(&_session);
        self.ready = NO;
    }
    if (!_sessionInitialized) self.statusText = transport.statusText;
    [self notifyDelegate];
}

- (void)beginPortableSession
{
    MblinkTransport transport = MBLinkBLETransportMakeCTransport(_provider);
    if (!mblink_transport_is_valid(&transport) || !mblink_elm327_session_init(&_session, &transport, JagLinkSessionEvent, (__bridge void *)self)) {
        _phase = JagLinkDiagnosticsPhaseFailed;
        [self setStatus:@"Failed to initialise portable diagnostic session"];
        return;
    }
    _sessionInitialized = YES;
    if (!_recorder.started && !mblink_telemetry_recorder_begin(&_recorder, &_sessionMetadata, JagLinkAppendCSV, (__bridge void *)_sessionCSV)) {
        _sessionInitialized = NO;
        mblink_elm327_session_disconnect(&_session);
        mblink_elm327_session_deinit(&_session);
        _phase = JagLinkDiagnosticsPhaseFailed;
        [self setStatus:@"Could not start portable session recorder"];
        return;
    }
    mblink_elm327_init_begin(&_initialization);
    _phase = JagLinkDiagnosticsPhaseInitializing;
    [self startTickTimer];
    [self setStatus:@"Initialising ELM327 adapter"];
    [self beginCurrentInitializationCommand];
}

- (void)startTickTimer
{
    [self stopTickTimer];
    _tickTimer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0U, 0U, dispatch_get_main_queue());
    dispatch_source_set_timer(_tickTimer, dispatch_time(DISPATCH_TIME_NOW, 100 * NSEC_PER_MSEC), 100 * NSEC_PER_MSEC, 20 * NSEC_PER_MSEC);
    __weak JagLinkDiagnosticsController *weakSelf = self;
    dispatch_source_set_event_handler(_tickTimer, ^{
        JagLinkDiagnosticsController *strongSelf = weakSelf;
        if (strongSelf == nil || !strongSelf->_sessionInitialized) return;
        (void)mblink_elm327_session_tick(&strongSelf->_session, JagLinkMonotonicMilliseconds());
    });
    dispatch_resume(_tickTimer);
}

- (void)stopTickTimer
{
    if (_tickTimer != nil) { dispatch_source_cancel(_tickTimer); _tickTimer = nil; }
}

- (BOOL)beginCommand:(const char *)command timeout:(uint64_t)timeoutMs
{
    if (!_sessionInitialized || command == NULL) return NO;
    MblinkElm327SessionOpResult result = mblink_elm327_session_begin(&_session, command, JagLinkMonotonicMilliseconds(), timeoutMs);
    if (result != MBLINK_ELM327_SESSION_OP_OK) {
        _phase = JagLinkDiagnosticsPhaseFailed;
        [self setStatus:[NSString stringWithFormat:@"Diagnostic command failed: %@", JagLinkStringFromCString(mblink_elm327_session_op_result_name(result))]];
        return NO;
    }
    return YES;
}

- (void)beginCurrentInitializationCommand
{
    const char *command = mblink_elm327_init_command(&_initialization);
    if (command == NULL) { _phase = JagLinkDiagnosticsPhaseFailed; [self setStatus:@"ELM327 initialisation state is invalid"]; return; }
    (void)[self beginCommand:command timeout:4000U];
}

- (void)handleSessionEvent:(const MblinkElm327Session *)session
{
    if (session->status == MBLINK_ELM327_SESSION_COMPLETE) { dispatch_async(dispatch_get_main_queue(), ^{ [self processCompletedResponse]; }); return; }
    if (session->status == MBLINK_ELM327_SESSION_TIMED_OUT) {
        if (_phase == JagLinkDiagnosticsPhaseScanningStoredDTCs || _phase == JagLinkDiagnosticsPhaseScanningPendingDTCs || _phase == JagLinkDiagnosticsPhaseScanningPermanentDTCs) self.faultScanStatusText = @"Fault scan timed out; reconnect required";
        _phase = JagLinkDiagnosticsPhaseFailed;
        [self setStatus:@"Diagnostic request timed out; reconnect to resynchronise"];
        return;
    }
    if (session->status == MBLINK_ELM327_SESSION_FAILED) {
        NSString *reason = JagLinkStringFromCString(mblink_elm327_result_name(session->elm_result));
        if (_phase == JagLinkDiagnosticsPhaseScanningStoredDTCs || _phase == JagLinkDiagnosticsPhaseScanningPendingDTCs || _phase == JagLinkDiagnosticsPhaseScanningPermanentDTCs) self.faultScanStatusText = [NSString stringWithFormat:@"Fault scan adapter error: %@", reason];
        _phase = JagLinkDiagnosticsPhaseFailed;
        [self setStatus:[NSString stringWithFormat:@"Adapter response failed: %@", reason]];
        return;
    }
    if (session->status == MBLINK_ELM327_SESSION_CANCELLED) { _phase = JagLinkDiagnosticsPhaseIdle; [self setStatus:@"Diagnostic request cancelled"]; }
}

- (void)processCompletedResponse
{
    const MblinkElm327Response *response = mblink_elm327_session_response(&_session);
    if (response == NULL) { _phase = JagLinkDiagnosticsPhaseFailed; [self setStatus:@"Diagnostic response was unavailable"]; return; }

    (void)mblink_telemetry_store_record_transcript(&_telemetry, JagLinkElapsedMilliseconds(_sessionMonotonicStartMs), _session.parser.command, response);
    if (_recorder.started && !_recorder.finished && !mblink_telemetry_recorder_record_response(&_recorder, JagLinkElapsedMilliseconds(_sessionMonotonicStartMs), _session.parser.command, response)) {
        _phase = JagLinkDiagnosticsPhaseFailed;
        [self setStatus:@"Could not append diagnostic transcript"];
        return;
    }

    switch (_phase) {
    case JagLinkDiagnosticsPhaseInitializing: [self processInitializationResponse:response]; break;
    case JagLinkDiagnosticsPhaseCheckingPids: [self processSupportedPidResponse:response]; break;
    case JagLinkDiagnosticsPhaseScanningStoredDTCs:
    case JagLinkDiagnosticsPhaseScanningPendingDTCs:
    case JagLinkDiagnosticsPhaseScanningPermanentDTCs: [self processFaultScanResponse:response]; break;
    case JagLinkDiagnosticsPhaseReadingLive: [self processLiveResponse:response]; break;
    case JagLinkDiagnosticsPhaseIdle:
    case JagLinkDiagnosticsPhaseLive:
    case JagLinkDiagnosticsPhaseFailed: break;
    }
}

- (void)processInitializationResponse:(const MblinkElm327Response *)response
{
    MblinkElm327Result result = mblink_elm327_init_accept(&_initialization, response);
    if (result != MBLINK_ELM327_RESULT_OK || _initialization.stage == MBLINK_ELM327_INIT_FAILED) { _phase = JagLinkDiagnosticsPhaseFailed; [self setStatus:@"ELM327 initialisation failed"]; return; }
    if (_initialization.adapter_id[0] != '\0') {
        self.adapterIdentifier = [NSString stringWithUTF8String:_initialization.adapter_id];
        mblink_telemetry_session_metadata_set_adapter(&_sessionMetadata, _initialization.adapter_id);
    }
    if (_initialization.stage == MBLINK_ELM327_INIT_COMPLETE) {
        char command[8];
        _supportedPidBase = 0x00U;
        if (mblink_obd2_build_supported_pid_request(_supportedPidBase, command, sizeof(command)) != MBLINK_OBD2_RESULT_OK) { _phase = JagLinkDiagnosticsPhaseFailed; [self setStatus:@"Could not build OBD-II capability request"]; return; }
        _phase = JagLinkDiagnosticsPhaseCheckingPids;
        [self setStatus:@"Checking standard OBD-II capabilities"];
        (void)[self beginCommand:command timeout:3000U];
        return;
    }
    [self beginCurrentInitializationCommand];
}

- (void)processSupportedPidResponse:(const MblinkElm327Response *)response
{
    bool hasMore = false;
    MblinkObd2Result result = mblink_obd2_accept_supported_pids(response, _supportedPidBase, &_supportedPids, &hasMore);
    if (result != MBLINK_OBD2_RESULT_OK) { _phase = JagLinkDiagnosticsPhaseFailed; [self setStatus:@"Vehicle did not provide a valid OBD-II PID map"]; return; }
    if (hasMore && _supportedPidBase <= 0xc0U) {
        char command[8];
        _supportedPidBase = (uint8_t)(_supportedPidBase + 0x20U);
        if (mblink_obd2_build_supported_pid_request(_supportedPidBase, command, sizeof(command)) != MBLINK_OBD2_RESULT_OK) { _phase = JagLinkDiagnosticsPhaseFailed; [self setStatus:@"Could not continue OBD-II capability discovery"]; return; }
        [self setStatus:[NSString stringWithFormat:@"Checking OBD-II PID block 0x%02X", (unsigned int)_supportedPidBase]];
        (void)[self beginCommand:command timeout:3000U];
        return;
    }
    [self beginFaultScan];
}

- (void)beginFaultScan
{
    self.storedDTCs = @[]; self.pendingDTCs = @[]; self.permanentDTCs = @[];
    self.faultScanStatusText = @"Scanning stored, pending and permanent OBD-II faults";
    [self beginFaultScanKind:MBLINK_OBD2_DTC_STORED];
}

- (void)beginFaultScanKind:(MblinkObd2DtcKind)kind
{
    char command[8];
    if (mblink_obd2_build_dtc_request(kind, command, sizeof(command)) != MBLINK_OBD2_RESULT_OK) { self.faultScanStatusText = @"Could not build OBD-II fault request"; [self completeLiveSetup]; return; }
    switch (kind) {
    case MBLINK_OBD2_DTC_STORED: _phase = JagLinkDiagnosticsPhaseScanningStoredDTCs; [self setStatus:@"Scanning stored OBD-II fault codes"]; break;
    case MBLINK_OBD2_DTC_PENDING: _phase = JagLinkDiagnosticsPhaseScanningPendingDTCs; [self setStatus:@"Scanning pending OBD-II fault codes"]; break;
    case MBLINK_OBD2_DTC_PERMANENT: _phase = JagLinkDiagnosticsPhaseScanningPermanentDTCs; [self setStatus:@"Scanning permanent OBD-II fault codes"]; break;
    }
    (void)[self beginCommand:command timeout:3000U];
}

- (void)processFaultScanResponse:(const MblinkElm327Response *)response
{
    MblinkObd2DtcKind kind;
    if (_phase == JagLinkDiagnosticsPhaseScanningStoredDTCs) kind = MBLINK_OBD2_DTC_STORED;
    else if (_phase == JagLinkDiagnosticsPhaseScanningPendingDTCs) kind = MBLINK_OBD2_DTC_PENDING;
    else if (_phase == JagLinkDiagnosticsPhaseScanningPermanentDTCs) kind = MBLINK_OBD2_DTC_PERMANENT;
    else { _phase = JagLinkDiagnosticsPhaseFailed; [self setStatus:@"Fault scan state was invalid"]; return; }

    MblinkObd2DtcList list = {0};
    MblinkObd2Result result = MBLINK_OBD2_RESULT_OK;
    if (response->result != MBLINK_ELM327_RESULT_NO_DATA) result = mblink_obd2_decode_dtcs(response, kind, &list);
    if (result != MBLINK_OBD2_RESULT_OK) { self.faultScanStatusText = [NSString stringWithFormat:@"Fault scan decode stopped: %@", JagLinkStringFromCString(mblink_obd2_result_name(result))]; [self completeLiveSetup]; return; }

    NSArray<NSString *> *codes = JagLinkDTCStrings(&list);
    switch (kind) {
    case MBLINK_OBD2_DTC_STORED: self.storedDTCs = codes; [self beginFaultScanKind:MBLINK_OBD2_DTC_PENDING]; return;
    case MBLINK_OBD2_DTC_PENDING: self.pendingDTCs = codes; [self beginFaultScanKind:MBLINK_OBD2_DTC_PERMANENT]; return;
    case MBLINK_OBD2_DTC_PERMANENT:
        self.permanentDTCs = codes;
        self.faultScanStatusText = [NSString stringWithFormat:@"Complete · %lu stored · %lu pending · %lu permanent", (unsigned long)self.storedDTCs.count, (unsigned long)self.pendingDTCs.count, (unsigned long)self.permanentDTCs.count];
        [self completeLiveSetup]; return;
    }
}

- (void)completeLiveSetup
{
    if (mblink_scheduler_configure_standard_obd2(&_scheduler, &_supportedPids, JagLinkMonotonicMilliseconds()) != MBLINK_SCHEDULER_RESULT_OK) { _phase = JagLinkDiagnosticsPhaseFailed; [self setStatus:@"Could not configure portable live-data scheduler"]; return; }
    self.ready = YES;
    _phase = JagLinkDiagnosticsPhaseLive;
    [self notifyDelegate];
    if (_scheduler.count == 0U) { [self setStatus:@"Connected; no supported dashboard PIDs were advertised"]; return; }
    [self setStatus:@"Live standard OBD-II data; X400 manufacturer discovery not yet enabled"];
    [self scheduleNextLiveRequest];
}

- (void)scheduleNextLiveRequest
{
    if (!_sessionInitialized || !_provider.isReady || _phase == JagLinkDiagnosticsPhaseFailed) return;
    MblinkSchedulerDispatch scheduled;
    MblinkSchedulerNextResult next = mblink_scheduler_next(&_scheduler, JagLinkMonotonicMilliseconds(), &scheduled);
    if (next == MBLINK_SCHEDULER_NEXT_EMPTY || next == MBLINK_SCHEDULER_NEXT_PAUSED) { _phase = JagLinkDiagnosticsPhaseLive; return; }
    if (next == MBLINK_SCHEDULER_NEXT_WAITING) {
        uint64_t waitMs = scheduled.wait_ms > 60000U ? 60000U : scheduled.wait_ms;
        const NSUInteger generation = _pollGeneration;
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)waitMs * NSEC_PER_MSEC), dispatch_get_main_queue(), ^{ if (generation == self->_pollGeneration) [self scheduleNextLiveRequest]; });
        return;
    }
    char command[8];
    if (mblink_obd2_build_live_pid_request(scheduled.pid, command, sizeof(command)) != MBLINK_OBD2_RESULT_OK) { _phase = JagLinkDiagnosticsPhaseFailed; [self setStatus:@"Could not build scheduled OBD-II request"]; return; }
    _activePid = scheduled.pid;
    _activeScheduleIndex = scheduled.index;
    _phase = JagLinkDiagnosticsPhaseReadingLive;
    const uint64_t nowMs = JagLinkMonotonicMilliseconds();
    if ([self beginCommand:command timeout:2000U]) (void)mblink_scheduler_mark_dispatched(&_scheduler, _activeScheduleIndex, nowMs);
}

- (void)processLiveResponse:(const MblinkElm327Response *)response
{
    if (response->result == MBLINK_ELM327_RESULT_NO_DATA) { _phase = JagLinkDiagnosticsPhaseLive; self.statusText = @"Live OBD-II data; one PID returned no data"; [self notifyDelegate]; [self scheduleNextLiveRequest]; return; }
    MblinkObd2Sample sample;
    MblinkObd2Result result = mblink_obd2_decode_live_pid(response, _activePid, &sample);
    if (result == MBLINK_OBD2_RESULT_UNSUPPORTED_PID) { _phase = JagLinkDiagnosticsPhaseLive; self.statusText = @"Live OBD-II data; one advertised sub-field is unavailable"; [self notifyDelegate]; [self scheduleNextLiveRequest]; return; }
    if (result != MBLINK_OBD2_RESULT_OK) { _phase = JagLinkDiagnosticsPhaseFailed; [self setStatus:[NSString stringWithFormat:@"Live OBD-II decode failed: %@", JagLinkStringFromCString(mblink_obd2_result_name(result))]]; return; }
    if (!mblink_telemetry_store_record(&_telemetry, JagLinkElapsedMilliseconds(_sessionMonotonicStartMs), &sample)) { _phase = JagLinkDiagnosticsPhaseFailed; [self setStatus:@"Could not record live telemetry sample"]; return; }
    MblinkTelemetrySample recorded;
    if (_recorder.started && !_recorder.finished && mblink_telemetry_store_latest(&_telemetry, sample.pid, &recorded) && !mblink_telemetry_recorder_record_sample(&_recorder, &recorded, mblink_telemetry_store_is_favourite(&_telemetry, sample.pid))) { _phase = JagLinkDiagnosticsPhaseFailed; [self setStatus:@"Could not append session recording"]; return; }
    self.ready = YES;
    _phase = JagLinkDiagnosticsPhaseLive;
    self.statusText = @"Live standard OBD-II data";
    [self notifyDelegate];
    [self scheduleNextLiveRequest];
}

- (NSUInteger)recordedSampleCount
{
    uint64_t total = mblink_telemetry_store_total_sample_count(&_telemetry);
    return total > (uint64_t)NSUIntegerMax ? NSUIntegerMax : (NSUInteger)total;
}

- (NSArray<NSNumber *> *)recentValuesForPID:(uint8_t)pid limit:(NSUInteger)limit
{
    if (limit == 0U) return @[];
    NSMutableArray<NSNumber *> *values = [[NSMutableArray alloc] initWithCapacity:limit];
    const size_t count = mblink_telemetry_store_history_count(&_telemetry);
    for (size_t reverseIndex = count; reverseIndex > 0U && values.count < limit; --reverseIndex) {
        MblinkTelemetrySample sample;
        if (!mblink_telemetry_store_history_at(&_telemetry, reverseIndex - 1U, &sample) || sample.measurement.pid != pid) continue;
        [values insertObject:@(sample.measurement.value) atIndex:0U];
    }
    return values;
}

- (BOOL)favouriteForPID:(uint8_t)pid { return mblink_telemetry_store_is_favourite(&_telemetry, pid); }
- (void)setFavourite:(BOOL)favourite forPID:(uint8_t)pid { mblink_telemetry_store_set_favourite(&_telemetry, pid, favourite); [self notifyDelegate]; }
- (nullable NSString *)csvSnapshot
{
    if (_sessionCSV.length == 0U) return nil;
    return [[NSString alloc] initWithData:[_sessionCSV copy] encoding:NSUTF8StringEncoding];
}

@end
