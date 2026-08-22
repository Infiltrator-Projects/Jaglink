// SPDX-License-Identifier: GPL-3.0-or-later
#import "JagLinkDiagnosticsController.h"

#import "JagLinkBLETransport+JAGLINK.h"
#import "jaglink/elm327.h"
#import "jaglink/elm327_session.h"
#import "jaglink/telemetry.h"
#import "link/diagnostic_flow.h"

#include <stdint.h>

@interface JagLinkDiagnosticsController () <JagLinkBLETransportDelegate>
@property(nonatomic, copy, readwrite) NSString *statusText;
@property(nonatomic, copy, readwrite, nullable) NSString *peripheralName;
@property(nonatomic, copy, readwrite, nullable) NSString *adapterIdentifier;
@property(nonatomic, copy, readwrite) NSString *faultScanStatusText;
@property(nonatomic, copy, readwrite) NSArray<NSString *> *storedDTCs;
@property(nonatomic, copy, readwrite) NSArray<NSString *> *pendingDTCs;
@property(nonatomic, copy, readwrite) NSArray<NSString *> *permanentDTCs;
@property(nonatomic, readwrite, getter=isActive) BOOL active;
@property(nonatomic, readwrite, getter=isReady) BOOL ready;

- (void)handleSessionEvent:(const JaglinkElm327Session *)session;
- (void)processCompletedResponse;
- (void)beginPortableSession;
- (BOOL)beginCommand:(const char *)command timeout:(uint64_t)timeoutMs;
- (void)startTickTimer;
- (void)stopTickTimer;
- (void)driveDiagnosticFlow;
- (BOOL)applyFlowEvent:(const LinkDiagnosticFlowEvent *)event;
- (void)markFlowFailure:(NSString *)status;
@end

@implementation JagLinkDiagnosticsController {
    JagLinkBLETransport *_provider;
    JaglinkElm327Session _session;
    BOOL _sessionInitialized;
    LinkDiagnosticFlow _flow;
    JaglinkTelemetryStore _telemetry;
    JaglinkTelemetryRecorder _recorder;
    JaglinkTelemetrySessionMetadata _sessionMetadata;
    NSMutableData *_sessionCSV;
    dispatch_source_t _tickTimer;
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

static void JagLinkSessionEvent(void *context, const JaglinkElm327Session *session)
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

static NSArray<NSString *> *JagLinkDTCStrings(const LinkObd2DtcList *list)
{
    if (list == NULL || list->count == 0U) return @[];
    NSMutableArray<NSString *> *values = [[NSMutableArray alloc] initWithCapacity:list->count];
    for (size_t index = 0U; index < list->count; ++index) {
        NSString *code = JagLinkStringFromCString(list->entries[index].code);
        if (code.length != 0U) [values addObject:code];
    }
    return [values copy];
}

static BOOL JagLinkFlowIsFaultScan(const LinkDiagnosticFlow *flow)
{
    if (flow == NULL) return NO;
    return flow->stage == LINK_DIAGNOSTIC_FLOW_SCANNING_STORED_DTCS ||
           flow->stage == LINK_DIAGNOSTIC_FLOW_SCANNING_PENDING_DTCS ||
           flow->stage == LINK_DIAGNOSTIC_FLOW_SCANNING_PERMANENT_DTCS;
}

- (instancetype)init
{
    self = [super init];
    if (self != nil) {
        _provider = [[JagLinkBLETransport alloc] init];
        _provider.delegate = self;
        _statusText = @"Idle";
        _faultScanStatusText = @"Not scanned";
        _storedDTCs = @[];
        _pendingDTCs = @[];
        _permanentDTCs = @[];
        LinkDiagnosticFlowConfig flowConfig = LINK_DIAGNOSTIC_FLOW_CONFIG_INIT;
        (void)link_diagnostic_flow_init(&_flow, &flowConfig);
        jaglink_telemetry_store_init(&_telemetry);
        jaglink_telemetry_recorder_init(&_recorder);
        _sessionCSV = [[NSMutableData alloc] init];
        jaglink_telemetry_store_set_favourite(&_telemetry, 0x0cU, true);
        jaglink_telemetry_store_set_favourite(&_telemetry, 0x0dU, true);
        jaglink_telemetry_store_set_favourite(&_telemetry, 0x05U, true);
        jaglink_telemetry_store_set_favourite(&_telemetry, 0x0bU, true);
        jaglink_telemetry_session_metadata_init(&_sessionMetadata, 0U, NULL, NULL);
    }
    return self;
}

- (void)dealloc
{
    _provider.delegate = nil;
    [self stopTickTimer];
    if (_recorder.started && !_recorder.finished) {
        (void)jaglink_telemetry_recorder_finish(&_recorder, JagLinkEpochMilliseconds());
    }
    if (_sessionInitialized) {
        _sessionInitialized = NO;
        jaglink_elm327_session_disconnect(&_session);
        jaglink_elm327_session_deinit(&_session);
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
    if (![NSThread isMainThread]) {
        dispatch_async(dispatch_get_main_queue(), ^{ [self start]; });
        return;
    }
    if (self.active) return;

    _pollGeneration++;
    self.active = YES;
    self.ready = NO;
    self.faultScanStatusText = @"Waiting for vehicle connection";
    self.storedDTCs = @[];
    self.pendingDTCs = @[];
    self.permanentDTCs = @[];
    LinkDiagnosticFlowConfig flowConfig = LINK_DIAGNOSTIC_FLOW_CONFIG_INIT;
    (void)link_diagnostic_flow_init(&_flow, &flowConfig);
    jaglink_telemetry_store_clear_samples(&_telemetry);
    jaglink_telemetry_recorder_init(&_recorder);
    _sessionCSV = [[NSMutableData alloc] init];
    _sessionMonotonicStartMs = JagLinkMonotonicMilliseconds();
    jaglink_telemetry_session_metadata_init(&_sessionMetadata, JagLinkEpochMilliseconds(), NULL, NULL);
    [self notifyDelegate];
    [_provider start];
}

- (void)disconnect
{
    if (![NSThread isMainThread]) {
        dispatch_async(dispatch_get_main_queue(), ^{ [self disconnect]; });
        return;
    }

    _pollGeneration++;
    [self stopTickTimer];
    if (_sessionInitialized) {
        _sessionInitialized = NO;
        jaglink_elm327_session_disconnect(&_session);
        jaglink_elm327_session_deinit(&_session);
    } else {
        [_provider disconnect];
    }
    const uint64_t endedEpochMs = JagLinkEpochMilliseconds();
    jaglink_telemetry_session_metadata_finish(&_sessionMetadata, endedEpochMs);
    if (_recorder.started && !_recorder.finished) {
        (void)jaglink_telemetry_recorder_finish(&_recorder, endedEpochMs);
    }
    LinkDiagnosticFlowConfig flowConfig = LINK_DIAGNOSTIC_FLOW_CONFIG_INIT;
    (void)link_diagnostic_flow_init(&_flow, &flowConfig);
    self.active = NO;
    self.ready = NO;
    [self setStatus:@"Disconnected"];
}

- (void)bleTransportDidUpdate:(JagLinkBLETransport *)transport
{
    self.peripheralName = transport.peripheralName;
    self.adapterIdentifier = transport.adapterIdentifier;
    if (transport.adapterIdentifier != nil) {
        jaglink_telemetry_session_metadata_set_adapter(&_sessionMetadata, transport.adapterIdentifier.UTF8String);
    }

    if (transport.isReady && !_sessionInitialized) {
        [self beginPortableSession];
        return;
    }
    if (!transport.isReady && _sessionInitialized && transport.state != JagLinkBLETransportStateProbing) {
        _pollGeneration++;
        [self stopTickTimer];
        _sessionInitialized = NO;
        jaglink_elm327_session_deinit(&_session);
        link_diagnostic_flow_fail(&_flow, LINK_DIAGNOSTIC_FLOW_RESULT_ELM_ERROR);
        self.ready = NO;
    }
    if (!_sessionInitialized) self.statusText = transport.statusText;
    [self notifyDelegate];
}

- (void)beginPortableSession
{
    JaglinkTransport transport = JagLinkBLETransportMakeCTransport(_provider);
    if (!jaglink_transport_is_valid(&transport) ||
        !jaglink_elm327_session_init(&_session, &transport, JagLinkSessionEvent, (__bridge void *)self)) {
        [self markFlowFailure:@"Failed to initialise portable diagnostic session"];
        return;
    }

    _sessionInitialized = YES;
    if (!_recorder.started &&
        !jaglink_telemetry_recorder_begin(&_recorder, &_sessionMetadata,
                                          JagLinkAppendCSV, (__bridge void *)_sessionCSV)) {
        _sessionInitialized = NO;
        jaglink_elm327_session_disconnect(&_session);
        jaglink_elm327_session_deinit(&_session);
        [self markFlowFailure:@"Could not start portable session recorder"];
        return;
    }

    LinkDiagnosticFlowConfig flowConfig = LINK_DIAGNOSTIC_FLOW_CONFIG_INIT;
    (void)link_diagnostic_flow_init(&_flow, &flowConfig);
    if (link_diagnostic_flow_start(&_flow) != LINK_DIAGNOSTIC_FLOW_RESULT_OK) {
        [self markFlowFailure:@"Could not start shared diagnostic flow"];
        return;
    }
    [self startTickTimer];
    [self setStatus:@"Initialising ELM327 adapter"];
    [self driveDiagnosticFlow];
}

- (void)startTickTimer
{
    [self stopTickTimer];
    _tickTimer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0U, 0U, dispatch_get_main_queue());
    dispatch_source_set_timer(_tickTimer,
                              dispatch_time(DISPATCH_TIME_NOW, 100 * NSEC_PER_MSEC),
                              100 * NSEC_PER_MSEC,
                              20 * NSEC_PER_MSEC);
    __weak JagLinkDiagnosticsController *weakSelf = self;
    dispatch_source_set_event_handler(_tickTimer, ^{
        JagLinkDiagnosticsController *strongSelf = weakSelf;
        if (strongSelf == nil || !strongSelf->_sessionInitialized) return;
        (void)jaglink_elm327_session_tick(&strongSelf->_session, JagLinkMonotonicMilliseconds());
    });
    dispatch_resume(_tickTimer);
}

- (void)stopTickTimer
{
    if (_tickTimer != nil) {
        dispatch_source_cancel(_tickTimer);
        _tickTimer = nil;
    }
}

- (BOOL)beginCommand:(const char *)command timeout:(uint64_t)timeoutMs
{
    if (!_sessionInitialized || command == NULL) return NO;
    JaglinkElm327SessionOpResult result =
        jaglink_elm327_session_begin(&_session, command,
                                     JagLinkMonotonicMilliseconds(), timeoutMs);
    if (result != JAGLINK_ELM327_SESSION_OP_OK) {
        NSString *reason = JagLinkStringFromCString(
            jaglink_elm327_session_op_result_name(result));
        link_diagnostic_flow_fail(&_flow, LINK_DIAGNOSTIC_FLOW_RESULT_ELM_ERROR);
        [self setStatus:[NSString stringWithFormat:@"Diagnostic command failed: %@", reason]];
        return NO;
    }
    return YES;
}

- (void)handleSessionEvent:(const JaglinkElm327Session *)session
{
    if (session->status == JAGLINK_ELM327_SESSION_COMPLETE) {
        dispatch_async(dispatch_get_main_queue(), ^{ [self processCompletedResponse]; });
        return;
    }
    if (session->status == JAGLINK_ELM327_SESSION_TIMED_OUT) {
        if (JagLinkFlowIsFaultScan(&_flow)) {
            self.faultScanStatusText = @"Fault scan timed out; reconnect required";
        }
        _flow.elm_failure = session->elm_result;
        link_diagnostic_flow_fail(&_flow, LINK_DIAGNOSTIC_FLOW_RESULT_ELM_ERROR);
        [self setStatus:@"Diagnostic request timed out; reconnect to resynchronise"];
        return;
    }
    if (session->status == JAGLINK_ELM327_SESSION_FAILED) {
        NSString *reason = JagLinkStringFromCString(jaglink_elm327_result_name(session->elm_result));
        if (JagLinkFlowIsFaultScan(&_flow)) {
            self.faultScanStatusText = [NSString stringWithFormat:@"Fault scan adapter error: %@", reason];
        }
        _flow.elm_failure = session->elm_result;
        link_diagnostic_flow_fail(&_flow, LINK_DIAGNOSTIC_FLOW_RESULT_ELM_ERROR);
        [self setStatus:[NSString stringWithFormat:@"Adapter response failed: %@", reason]];
        return;
    }
    if (session->status == JAGLINK_ELM327_SESSION_CANCELLED) {
        LinkDiagnosticFlowConfig flowConfig = LINK_DIAGNOSTIC_FLOW_CONFIG_INIT;
        (void)link_diagnostic_flow_init(&_flow, &flowConfig);
        [self setStatus:@"Diagnostic request cancelled"];
    }
}

- (void)processCompletedResponse
{
    const JaglinkElm327Response *response = jaglink_elm327_session_response(&_session);
    if (response == NULL) {
        [self markFlowFailure:@"Diagnostic response was unavailable"];
        return;
    }

    (void)jaglink_telemetry_store_record_transcript(
        &_telemetry,
        JagLinkElapsedMilliseconds(_sessionMonotonicStartMs),
        _session.parser.command,
        response);
    if (_recorder.started && !_recorder.finished &&
        !jaglink_telemetry_recorder_record_response(
            &_recorder,
            JagLinkElapsedMilliseconds(_sessionMonotonicStartMs),
            _session.parser.command,
            response)) {
        [self markFlowFailure:@"Could not append diagnostic transcript"];
        return;
    }

    LinkDiagnosticFlowEvent event;
    LinkDiagnosticFlowResult result = link_diagnostic_flow_accept_response(
        &_flow,
        (const LinkElm327Response *)response,
        JagLinkMonotonicMilliseconds(),
        &event);
    if (result != LINK_DIAGNOSTIC_FLOW_RESULT_OK) {
        NSString *reason = JagLinkStringFromCString(link_diagnostic_flow_result_name(result));
        [self setStatus:[NSString stringWithFormat:@"Shared diagnostic flow failed: %@", reason]];
        return;
    }
    if (![self applyFlowEvent:&event]) return;
    [self driveDiagnosticFlow];
}

- (void)driveDiagnosticFlow
{
    if (!_sessionInitialized || !_provider.isReady ||
        _flow.stage == LINK_DIAGNOSTIC_FLOW_FAILED) {
        return;
    }

    LinkDiagnosticFlowAction action;
    LinkDiagnosticFlowResult result = link_diagnostic_flow_next_action(
        &_flow, JagLinkMonotonicMilliseconds(), &action);
    if (result != LINK_DIAGNOSTIC_FLOW_RESULT_OK) {
        NSString *reason = JagLinkStringFromCString(link_diagnostic_flow_result_name(result));
        [self setStatus:[NSString stringWithFormat:@"Shared diagnostic flow failed: %@", reason]];
        return;
    }

    switch (action.kind) {
    case LINK_DIAGNOSTIC_FLOW_ACTION_NONE:
        return;

    case LINK_DIAGNOSTIC_FLOW_ACTION_SEND_COMMAND:
        if (_flow.stage == LINK_DIAGNOSTIC_FLOW_INITIALIZING) {
            self.statusText = @"Initialising ELM327 adapter";
        } else if (_flow.stage == LINK_DIAGNOSTIC_FLOW_DISCOVERING_PIDS) {
            if (_flow.supported_pid_base == 0U) {
                self.statusText = @"Checking standard OBD-II capabilities";
            } else {
                self.statusText = [NSString stringWithFormat:
                    @"Checking OBD-II PID block 0x%02X",
                    (unsigned int)_flow.supported_pid_base];
            }
        } else if (_flow.stage == LINK_DIAGNOSTIC_FLOW_SCANNING_STORED_DTCS) {
            self.faultScanStatusText = @"Scanning stored, pending and permanent OBD-II faults";
            self.statusText = @"Scanning stored OBD-II fault codes";
        } else if (_flow.stage == LINK_DIAGNOSTIC_FLOW_SCANNING_PENDING_DTCS) {
            self.statusText = @"Scanning pending OBD-II fault codes";
        } else if (_flow.stage == LINK_DIAGNOSTIC_FLOW_SCANNING_PERMANENT_DTCS) {
            self.statusText = @"Scanning permanent OBD-II fault codes";
        } else if (_flow.stage == LINK_DIAGNOSTIC_FLOW_READING_LIVE) {
            self.statusText = @"Live standard OBD-II data; X400 manufacturer discovery not yet enabled";
        }
        [self notifyDelegate];
        (void)[self beginCommand:action.command timeout:action.timeout_ms];
        return;

    case LINK_DIAGNOSTIC_FLOW_ACTION_WAIT: {
        uint64_t waitMs = action.wait_ms > 60000U ? 60000U : action.wait_ms;
        const NSUInteger generation = _pollGeneration;
        dispatch_after(
            dispatch_time(DISPATCH_TIME_NOW, (int64_t)waitMs * NSEC_PER_MSEC),
            dispatch_get_main_queue(), ^{
                if (generation == self->_pollGeneration) [self driveDiagnosticFlow];
            });
        return;
    }

    case LINK_DIAGNOSTIC_FLOW_ACTION_READY:
        self.ready = YES;
        if (_flow.scheduler.count == 0U) {
            [self setStatus:@"Connected; no supported dashboard PIDs were advertised"];
        } else {
            [self setStatus:@"Live standard OBD-II data; X400 manufacturer discovery not yet enabled"];
        }
        return;

    case LINK_DIAGNOSTIC_FLOW_ACTION_MANUFACTURER_EXTENSION:
        [self markFlowFailure:@"Unexpected manufacturer extension request in JAGLINK standard flow"];
        return;

    case LINK_DIAGNOSTIC_FLOW_ACTION_FAILED:
        [self markFlowFailure:@"Shared diagnostic flow entered the failed state"];
        return;
    }
}

- (BOOL)applyFlowEvent:(const LinkDiagnosticFlowEvent *)event
{
    if (event == NULL) return NO;

    switch (event->kind) {
    case LINK_DIAGNOSTIC_FLOW_EVENT_NONE:
        return YES;

    case LINK_DIAGNOSTIC_FLOW_EVENT_ADAPTER_IDENTIFIED: {
        const char *identifier = link_diagnostic_flow_adapter_identifier(&_flow);
        if (identifier != NULL) {
            self.adapterIdentifier = JagLinkStringFromCString(identifier);
            jaglink_telemetry_session_metadata_set_adapter(&_sessionMetadata, identifier);
        }
        return YES;
    }

    case LINK_DIAGNOSTIC_FLOW_EVENT_PID_DISCOVERY_COMPLETE:
        return YES;

    case LINK_DIAGNOSTIC_FLOW_EVENT_DTC_LIST: {
        NSArray<NSString *> *codes = JagLinkDTCStrings(event->dtc_list);
        switch (event->dtc_kind) {
        case LINK_OBD2_DTC_STORED:
            self.storedDTCs = codes;
            break;
        case LINK_OBD2_DTC_PENDING:
            self.pendingDTCs = codes;
            break;
        case LINK_OBD2_DTC_PERMANENT:
            self.permanentDTCs = codes;
            self.faultScanStatusText = [NSString stringWithFormat:
                @"Complete · %lu stored · %lu pending · %lu permanent",
                (unsigned long)self.storedDTCs.count,
                (unsigned long)self.pendingDTCs.count,
                (unsigned long)self.permanentDTCs.count];
            break;
        }
        if (event->became_ready) self.ready = YES;
        [self notifyDelegate];
        return YES;
    }

    case LINK_DIAGNOSTIC_FLOW_EVENT_LIVE_SAMPLE: {
        if (!jaglink_telemetry_store_record(
                &_telemetry,
                JagLinkElapsedMilliseconds(_sessionMonotonicStartMs),
                &event->sample)) {
            [self markFlowFailure:@"Could not record live telemetry sample"];
            return NO;
        }
        JaglinkTelemetrySample recorded;
        if (_recorder.started && !_recorder.finished &&
            jaglink_telemetry_store_latest(&_telemetry, event->sample.pid, &recorded) &&
            !jaglink_telemetry_recorder_record_sample(
                &_recorder, &recorded,
                jaglink_telemetry_store_is_favourite(&_telemetry, event->sample.pid))) {
            [self markFlowFailure:@"Could not append session recording"];
            return NO;
        }
        self.ready = YES;
        self.statusText = @"Live standard OBD-II data";
        [self notifyDelegate];
        return YES;
    }

    case LINK_DIAGNOSTIC_FLOW_EVENT_LIVE_NO_DATA:
        self.statusText = @"Live OBD-II data; one PID returned no data";
        [self notifyDelegate];
        return YES;

    case LINK_DIAGNOSTIC_FLOW_EVENT_LIVE_UNSUPPORTED:
        self.statusText = @"Live OBD-II data; one advertised sub-field is unavailable";
        [self notifyDelegate];
        return YES;
    }
    return YES;
}

- (void)markFlowFailure:(NSString *)status
{
    link_diagnostic_flow_fail(&_flow, LINK_DIAGNOSTIC_FLOW_RESULT_INVALID_STATE);
    self.ready = NO;
    [self setStatus:status];
}

- (NSUInteger)recordedSampleCount
{
    uint64_t total = jaglink_telemetry_store_total_sample_count(&_telemetry);
    return total > (uint64_t)NSUIntegerMax ? NSUIntegerMax : (NSUInteger)total;
}

- (NSArray<NSNumber *> *)recentValuesForPID:(uint8_t)pid limit:(NSUInteger)limit
{
    if (limit == 0U) return @[];
    NSMutableArray<NSNumber *> *values = [[NSMutableArray alloc] initWithCapacity:limit];
    const size_t count = jaglink_telemetry_store_history_count(&_telemetry);
    for (size_t reverseIndex = count; reverseIndex > 0U && values.count < limit; --reverseIndex) {
        JaglinkTelemetrySample sample;
        if (!jaglink_telemetry_store_history_at(&_telemetry, reverseIndex - 1U, &sample) ||
            sample.measurement.pid != pid) {
            continue;
        }
        [values insertObject:@(sample.measurement.value) atIndex:0U];
    }
    return values;
}

- (BOOL)favouriteForPID:(uint8_t)pid
{
    return jaglink_telemetry_store_is_favourite(&_telemetry, pid);
}

- (void)setFavourite:(BOOL)favourite forPID:(uint8_t)pid
{
    jaglink_telemetry_store_set_favourite(&_telemetry, pid, favourite);
    [self notifyDelegate];
}

- (nullable NSString *)csvSnapshot
{
    if (_sessionCSV.length == 0U) return nil;
    return [[NSString alloc] initWithData:[_sessionCSV copy]
                                 encoding:NSUTF8StringEncoding];
}

@end
