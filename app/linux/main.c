// SPDX-License-Identifier: GPL-3.0-or-later
#include "about-dialog.h"
#include "jaglink/jaglink.h"
#include "jaglink/jaguar.h"
#include "jaglink/parameter.h"
#include "link-gtk-shell.h"
#include "link-gtk-widgets.h"
#include "link/fuel_economy.h"
#include "link/workspace.h"

#include <gtk/gtk.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void jaglink_linux_resources_register_resource(void);
void jaglink_linux_resources_unregister_resource(void);

typedef struct JaglinkLinuxContext {
    bool connected;
    char adapter_identity[160];
    LinkTransport transport;
    bool diagnostic_valid;
    bool diagnostic_active;
    bool diagnostic_ready;
    LinkDiagnosticFlow diagnostic;
    bool sample_valid[256];
    LinkObd2Sample samples[256];
    LinkFuelEconomy fuel_economy;
} JaglinkLinuxContext;

static const char jaglink_css[] =
    "window { background: #061a13; color: #eee8da; }"
    ".link-connection-bar { background: #0b2d21; border: 1px solid rgba(183,154,98,0.52); }"
    ".link-link-button { background: #b79a62; color: #071b14; border-radius: 10px; }"
    ".link-brand { color: #eee8da; font-family: serif; }"
    ".link-brand-subtitle { color: #b79a62; }"
    ".link-section-title { color: #eee8da; }"
    ".link-section-summary { color: #9fa9a2; }"
    ".link-card { background: linear-gradient(135deg,#0d3829,#09291f); border: 1px solid rgba(183,154,98,0.42); border-radius: 18px; padding: 20px; }"
    ".link-card-kicker { color: #b79a62; font-size: 10px; font-weight: 800; letter-spacing: 2px; }"
    ".link-card-title { color: #eee8da; font-family: serif; font-size: 20px; font-weight: 800; }"
    ".link-detail-label { color: #9fa9a2; }"
    ".link-detail-value { color: #eee8da; font-weight: 700; }"
    ".link-card-note { color: #c8c2b3; }"
    ".link-status-chip { padding: 7px 11px; border-radius: 999px; border: 1px solid rgba(183,154,98,0.5); font-weight: 700; }"
    ".state-warning { color: #d9bc7b; border-color: #8c7141; }"
    ".state-success { color: #87c99f; border-color: #48775a; }";

static uint64_t monotonic_ms(void)
{
    const gint64 value = g_get_monotonic_time();
    return value <= 0 ? 0U : (uint64_t)(value / 1000);
}

static const char *connection_text(const JaglinkLinuxContext *context)
{
    return context->connected ? "LINKED · ELM327 VERIFIED" : "NOT LINKED";
}

static const char *diagnostic_text(const JaglinkLinuxContext *context)
{
    if (!context->connected) return "LINK OFFLINE";
    if (!context->diagnostic_valid) return "STARTING DIAGNOSTICS";
    if (context->diagnostic.stage == LINK_DIAGNOSTIC_FLOW_FAILED) return "DIAGNOSTIC SESSION FAILED";
    if (context->diagnostic_ready) return "LIVE DIAGNOSTICS ACTIVE";
    return link_diagnostic_flow_stage_name(context->diagnostic.stage);
}

static const char *fuel_source_text(LinkFuelEconomySource source)
{
    switch (source) {
    case LINK_FUEL_ECONOMY_SOURCE_FACTORY_DIRECT: return "Jaguar factory direct";
    case LINK_FUEL_ECONOMY_SOURCE_FACTORY_COUNTERS: return "Jaguar factory counters";
    case LINK_FUEL_ECONOMY_SOURCE_FACTORY_RATE: return "Jaguar factory fuel rate";
    case LINK_FUEL_ECONOMY_SOURCE_SAE_OBD2: return "SAE OBD-II PID 0x5E + 0x0D";
    case LINK_FUEL_ECONOMY_SOURCE_ESTIMATED: return "Estimated";
    case LINK_FUEL_ECONOMY_SOURCE_MIXED: return "Mixed measured sources";
    case LINK_FUEL_ECONOMY_SOURCE_NONE: return "Unavailable";
    }
    return "Unavailable";
}

static void format_sample(const LinkObd2Sample *sample,
                          char *buffer,
                          size_t capacity)
{
    const char *unit;
    if (buffer == NULL || capacity == 0U) return;
    if (sample == NULL) {
        (void)snprintf(buffer, capacity, "Waiting");
        return;
    }
    unit = link_obd2_unit_name(sample->unit);
    if (unit == NULL || unit[0] == '\0' || sample->unit == LINK_OBD2_UNIT_NONE)
        (void)snprintf(buffer, capacity, "%.2f", sample->value);
    else
        (void)snprintf(buffer, capacity, "%.2f %s", sample->value, unit);
}

static void append_dtc_list(GtkWidget *card,
                            const char *prefix,
                            const LinkObd2DtcList *list)
{
    size_t index;
    if (card == NULL || prefix == NULL || list == NULL) return;
    if (list->count == 0U) {
        char label[48];
        (void)snprintf(label, sizeof(label), "%s faults", prefix);
        link_gtk_card_append_detail(card, label, "None reported");
        return;
    }
    for (index = 0U; index < list->count; ++index) {
        char label[48];
        (void)snprintf(label, sizeof(label), "%s %zu", prefix, index + 1U);
        link_gtk_card_append_detail(card, label, list->entries[index].code);
    }
}

static void append_vehicle(GtkWidget *body, JaglinkLinuxContext *context)
{
    const JaglinkJaguarVehicleProfile *profile = jaglink_jaguar_x400_profile();
    const char *vin = context->diagnostic_valid
        ? link_diagnostic_flow_standard_vin(&context->diagnostic) : NULL;
    JaglinkJaguarVinDecode decoded;
    const bool vin_decoded =
        vin != NULL && jaglink_jaguar_vin_decode(vin, &decoded);
    GtkWidget *identity = link_gtk_card_new(
        "VEHICLE IDENTITY", "Jaguar VIN / X400 decoding");
    GtkWidget *connection = link_gtk_card_new(
        "CONNECTION", "Linux diagnostic link");
    char value[160];

    if (vin_decoded) {
        link_gtk_card_append_detail(identity, "VIN", decoded.vin);
        link_gtk_card_append_detail(
            identity, "Manufacturer",
            decoded.jaguar_wmi ? "Jaguar · SAJ" : decoded.wmi);
        link_gtk_card_append_detail(
            identity, "Platform",
            decoded.x400 ? "X400 · X-TYPE" : "Jaguar VIN · X400 not confirmed");

        if (decoded.market != NULL)
            link_gtk_card_append_detail(
                identity, "Market / restraint",
                decoded.market->market);

        if (decoded.body != NULL) {
            (void)snprintf(value, sizeof(value), "%s · %s",
                           jaglink_jaguar_body_style_name(
                               decoded.body->body_style),
                           decoded.body->series_class);
            link_gtk_card_append_detail(identity, "Body", value);
        } else {
            link_gtk_card_append_detail(
                identity, "Body code", decoded.body_code);
        }

        if (decoded.model_year != 0U) {
            (void)snprintf(value, sizeof(value), "%u",
                           (unsigned int)decoded.model_year);
            link_gtk_card_append_detail(identity, "Model year", value);
        }

        if (decoded.transmission_steering != NULL) {
            link_gtk_card_append_detail(
                identity, "Drivetrain",
                jaglink_jaguar_drivetrain_name(
                    decoded.transmission_steering->drivetrain));
            link_gtk_card_append_detail(
                identity, "Transmission",
                jaglink_jaguar_transmission_name(
                    decoded.transmission_steering->transmission));
            link_gtk_card_append_detail(
                identity, "Steering",
                jaglink_jaguar_steering_name(
                    decoded.transmission_steering->steering));
        }

        if (decoded.plant_engine != NULL) {
            link_gtk_card_append_detail(
                identity, "Engine",
                decoded.plant_engine->engine_description);
            link_gtk_card_append_detail(
                identity, "Engine family",
                decoded.plant_engine->engine_family);
            link_gtk_card_append_detail(
                identity, "Fuel",
                jaglink_jaguar_fuel_type_name(
                    decoded.plant_engine->fuel));
            (void)snprintf(value, sizeof(value), "%u cc",
                           decoded.plant_engine->displacement_cc);
            link_gtk_card_append_detail(identity, "Displacement", value);
            if (decoded.plant_engine->rated_power_kw != 0U) {
                (void)snprintf(value, sizeof(value), "%u kW",
                               decoded.plant_engine->rated_power_kw);
                link_gtk_card_append_detail(
                    identity, "Catalogue power", value);
            }
            (void)snprintf(value, sizeof(value), "%s, %s",
                           decoded.plant_engine->assembly_plant,
                           decoded.plant_engine->assembly_country);
            link_gtk_card_append_detail(identity, "Assembly", value);
        } else {
            value[0] = decoded.plant_engine_code;
            value[1] = '\0';
            link_gtk_card_append_detail(
                identity, "Plant / engine-line code", value);
        }

        if (decoded.emission != NULL) {
            (void)snprintf(value, sizeof(value), "ECS %u",
                           decoded.emission->ecs_number);
            link_gtk_card_append_detail(identity, "Emissions", value);
        } else {
            value[0] = decoded.emission_code;
            value[1] = '\0';
            link_gtk_card_append_detail(identity, "Emissions code", value);
        }
        link_gtk_card_append_detail(
            identity, "Production serial", decoded.production_serial);
    } else if (context->diagnostic_valid &&
               context->diagnostic.standard_vin_attempted) {
        link_gtk_card_append_status(
            identity,
            "STANDARD VIN NOT RETURNED · DIAGNOSTICS CONTINUE",
            "state-warning");
        link_gtk_card_append_note(
            identity,
            "Some early X400 vehicles may not return SAE Mode 09 PID 02. "
            "JAGLINK treats that as missing identity evidence, not a diagnostic failure.");
    } else {
        link_gtk_card_append_status(
            identity, "WAITING FOR SAE MODE 09 VIN", "state-warning");
    }

    if (profile != NULL) {
        (void)snprintf(value, sizeof(value), "%u–%u · %zu networks · %zu factory routes",
                       (unsigned int)profile->first_model_year,
                       (unsigned int)profile->last_model_year,
                       profile->network_count,
                       profile->diagnostic_endpoint_count);
        link_gtk_card_append_detail(identity, "X400 knowledge", value);
    }

    link_gtk_card_append_status(
        connection, connection_text(context),
        context->connected ? "state-success" : "state-warning");
    link_gtk_card_append_detail(
        connection, "Adapter",
        context->connected && context->adapter_identity[0] != '\0'
            ? context->adapter_identity
            : "Select an adapter above and press LINK UP");
    link_gtk_card_append_detail(
        connection, "Diagnostic flow", diagnostic_text(context));
    link_gtk_card_append_note(
        connection,
        "LINK acquires the standard VIN and owns generic OBD-II transport. "
        "JAGLINK decodes Jaguar's X400-specific market, body, drivetrain, "
        "steering, model-year and Halewood engine-line fields offline.");

    gtk_box_append(GTK_BOX(body), identity);
    gtk_box_append(GTK_BOX(body), connection);
}

static void append_modules(GtkWidget *body)
{
    const JaglinkJaguarVehicleProfile *profile = jaglink_jaguar_x400_profile();
    GtkWidget *topology = link_gtk_card_new("X400 NETWORK TOPOLOGY", "Diagnostic networks and module paths");
    GtkWidget *factory = link_gtk_card_new("FACTORY DIAGNOSTIC ROUTES", "Jaguar documented CAN request / response channels");
    size_t index;

    if (profile == NULL || profile->network_count == 0U) {
        link_gtk_card_append_status(topology, "NO NETWORK DEFINITIONS", "state-warning");
    } else {
        for (index = 0U; index < profile->network_count; ++index) {
            const JaglinkJaguarNetworkDefinition *network = &profile->networks[index];
            char detail[128];
            if (network->nominal_baud != 0U)
                (void)snprintf(detail, sizeof(detail), "%s · %s · %u bit/s · %s",
                               jaglink_jaguar_network_kind_name(network->kind),
                               jaglink_jaguar_network_role_name(network->role),
                               (unsigned int)network->nominal_baud,
                               jaglink_jaguar_definition_status_name(network->status));
            else
                (void)snprintf(detail, sizeof(detail), "%s · %s · %s",
                               jaglink_jaguar_network_kind_name(network->kind),
                               jaglink_jaguar_network_role_name(network->role),
                               jaglink_jaguar_definition_status_name(network->status));
            link_gtk_card_append_detail(topology, network->name, detail);
        }
    }
    link_gtk_card_append_note(topology,
        "Jaguar-specific topology remains in JAGLINK while the standards transport, fault and live-data engine is shared through LINK.");

    if (profile == NULL || profile->diagnostic_endpoint_count == 0U) {
        link_gtk_card_append_status(factory, "NO FACTORY ROUTES", "state-warning");
    } else {
        link_gtk_card_append_status(factory, "SOURCE-CORROBORATED X400 ROUTES LOADED", "state-success");
        for (index = 0U; index < profile->diagnostic_endpoint_count; ++index) {
            const JaglinkJaguarDiagnosticEndpointDefinition *endpoint = &profile->diagnostic_endpoints[index];
            char detail[128];
            (void)snprintf(detail, sizeof(detail),
                           "%s · CAN 0x%03X → 0x%03X · %s",
                           jaglink_jaguar_module_kind_name(endpoint->module),
                           (unsigned int)endpoint->request_message_id,
                           (unsigned int)endpoint->response_message_id,
                           jaglink_jaguar_definition_status_name(endpoint->status));
            link_gtk_card_append_detail(factory, endpoint->name, detail);
        }
    }
    link_gtk_card_append_note(factory,
        "These identifiers describe Jaguar's documented factory diagnostic routing. JAGLINK does not assume that a modern UDS payload is valid for X400; the read payload remains disabled until its request format is independently corroborated.");

    gtk_box_append(GTK_BOX(body), topology);
    gtk_box_append(GTK_BOX(body), factory);
}

static void append_factory_catalogue(GtkWidget *card,
                                     const JaglinkJaguarVehicleProfile *profile)
{
    size_t index;
    if (card == NULL || profile == NULL) return;
    for (index = 0U; index < profile->factory_dtc_count; ++index) {
        const JaglinkJaguarFactoryDtcDefinition *dtc = &profile->factory_dtcs[index];
        char detail[128];
        (void)snprintf(detail, sizeof(detail), "%s · %s · %s",
                       jaglink_jaguar_module_kind_name(dtc->module),
                       dtc->category,
                       dtc->generic_obd2_accessible ? "generic OBD accessible" : "Jaguar factory/WDS layer");
        link_gtk_card_append_detail(card, dtc->code, detail);
    }
}

static void append_faults(GtkWidget *body, const JaglinkLinuxContext *context)
{
    const JaglinkJaguarVehicleProfile *profile = jaglink_jaguar_x400_profile();
    GtkWidget *standard = link_gtk_card_new("STANDARD SAE OBD-II", "Stored, pending and permanent faults");
    GtkWidget *jaguar = link_gtk_card_new("JAGUAR FACTORY DTC CATALOGUE", "Manufacturer-only X400 fault identities");
    char summary[160];

    if (!context->connected) {
        link_gtk_card_append_status(standard, "NOT SCANNED · LINK OFFLINE", "state-warning");
    } else if (!context->diagnostic_valid) {
        link_gtk_card_append_status(standard, "STARTING SCAN", "state-warning");
    } else if (context->diagnostic.stage == LINK_DIAGNOSTIC_FLOW_FAILED) {
        link_gtk_card_append_status(standard, "SCAN FAILED · RECONNECT TO RETRY", "state-warning");
    } else if (context->diagnostic_ready) {
        (void)snprintf(summary, sizeof(summary),
                       "COMPLETE · %zu stored · %zu pending · %zu permanent",
                       context->diagnostic.stored_dtcs.count,
                       context->diagnostic.pending_dtcs.count,
                       context->diagnostic.permanent_dtcs.count);
        link_gtk_card_append_status(standard, summary, "state-success");
        append_dtc_list(standard, "Stored", &context->diagnostic.stored_dtcs);
        append_dtc_list(standard, "Pending", &context->diagnostic.pending_dtcs);
        append_dtc_list(standard, "Permanent", &context->diagnostic.permanent_dtcs);
    } else {
        (void)snprintf(summary, sizeof(summary), "SCAN IN PROGRESS · %s",
                       link_diagnostic_flow_stage_name(context->diagnostic.stage));
        link_gtk_card_append_status(standard, summary, "state-warning");
    }
    link_gtk_card_append_note(standard,
        "These are live standards-based powertrain DTCs from LINK. They are not used as a substitute for Jaguar's factory/module diagnostic view.");

    if (profile != NULL && profile->factory_dtc_count != 0U) {
        (void)snprintf(summary, sizeof(summary), "%zu SOURCE-CORROBORATED FACTORY CODE IDENTITIES LOADED",
                       profile->factory_dtc_count);
        link_gtk_card_append_status(jaguar, summary, "state-success");
        append_factory_catalogue(jaguar, profile);
    } else {
        link_gtk_card_append_status(jaguar, "NO FACTORY CATALOGUE", "state-warning");
    }
    link_gtk_card_append_note(jaguar,
        "Catalogue entries are known Jaguar factory DTC identities, not claims that those faults are present on the connected car. Jaguar's 2002 documentation distinguishes generic OBD-II codes from JAG codes read through WDS; live factory acquisition will only be enabled when the X400 read request itself is evidence-backed.");

    gtk_box_append(GTK_BOX(body), standard);
    gtk_box_append(GTK_BOX(body), jaguar);
}

static void append_parameters(GtkWidget *body,
                              bool compact,
                              const JaglinkLinuxContext *context)
{
    GtkWidget *card = link_gtk_card_new(compact ? "PARAMETER TABLE" : "LIVE DATA CATALOGUE",
                                        compact ? "Real standard OBD-II samples" : "Available shared diagnostic parameters");
    size_t count = jaglink_parameter_obd2_definition_count();
    size_t index;
    for (index = 0U; index < count; ++index) {
        const JaglinkParameterDefinition *definition = jaglink_parameter_obd2_definition_at(index);
        char key[64];
        char value[96];
        uint8_t pid;
        if (definition == NULL) continue;
        pid = (uint8_t)definition->key.identifier;
        (void)snprintf(key, sizeof(key), "PID 0x%02X · %s",
                       (unsigned int)pid, definition->short_name);
        if (context->sample_valid[pid]) {
            format_sample(&context->samples[pid], value, sizeof(value));
        } else if (context->diagnostic_valid &&
                   !link_obd2_pid_set_contains(&context->diagnostic.supported_pids, pid)) {
            (void)snprintf(value, sizeof(value), "Not supported by vehicle");
        } else if (context->diagnostic_active || context->diagnostic_ready) {
            (void)snprintf(value, sizeof(value), "Waiting for sample");
        } else {
            (void)snprintf(value, sizeof(value), "No live session");
        }
        link_gtk_card_append_detail(card, compact ? key : definition->name, value);
    }
    gtk_box_append(GTK_BOX(body), card);
}

static void append_fuel_economy(GtkWidget *body,
                                const JaglinkLinuxContext *context)
{
    LinkFuelEconomySnapshot snapshot =
        link_fuel_economy_snapshot(&context->fuel_economy, monotonic_ms());
    const JaglinkJaguarFuelSignalDefinition *factory =
        jaglink_jaguar_x400_find_fuel_signal("x400-can-fuel-used");
    GtkWidget *card = link_gtk_card_new("FUEL ECONOMY", "Fuel use and trip consumption");
    char instantaneous[64];
    char average[64];
    char rate[64];
    char trip[96];
    char factory_status[160];
    LinkFuelEconomySource display_source = snapshot.instantaneous_available
        ? snapshot.instantaneous_source
        : (snapshot.fuel_rate_available ? snapshot.fuel_rate_source : snapshot.average_source);

    if (snapshot.instantaneous_available) {
        (void)snprintf(instantaneous, sizeof(instantaneous), "%.1f L/100 km",
                       snapshot.instantaneous_l_per_100km);
    } else if (context->connected && !snapshot.moving) {
        (void)snprintf(instantaneous, sizeof(instantaneous), "— · stationary / awaiting speed");
    } else {
        (void)snprintf(instantaneous, sizeof(instantaneous), "Waiting for measured fuel data");
    }

    if (snapshot.average_available) {
        (void)snprintf(average, sizeof(average), "%.1f L/100 km",
                       snapshot.average_l_per_100km);
    } else {
        (void)snprintf(average, sizeof(average), "Waiting for trip distance");
    }

    if (snapshot.fuel_rate_available) {
        (void)snprintf(rate, sizeof(rate), "%.2f L/h", snapshot.fuel_rate_l_per_hour);
    } else {
        (void)snprintf(rate, sizeof(rate), "Not available");
    }

    (void)snprintf(trip, sizeof(trip), "%.2f L over %.1f km",
                   snapshot.trip_fuel_litres, snapshot.trip_distance_km);

    link_gtk_card_append_status(card,
        snapshot.instantaneous_available || snapshot.fuel_rate_available
            ? "MEASURED FUEL DATA ACTIVE" : "WAITING FOR FUEL DATA",
        snapshot.instantaneous_available || snapshot.fuel_rate_available
            ? "state-success" : "state-warning");
    link_gtk_card_append_detail(card, "Instantaneous", instantaneous);
    link_gtk_card_append_detail(card, "Trip average", average);
    link_gtk_card_append_detail(card, "Fuel rate", rate);
    link_gtk_card_append_detail(card, "Trip", trip);
    link_gtk_card_append_detail(card, "Current source", fuel_source_text(display_source));

    if (factory != NULL) {
        (void)snprintf(factory_status, sizeof(factory_status),
                       "CAN 0x%03X · %s · %s",
                       (unsigned int)factory->message_id,
                       jaglink_jaguar_definition_status_name(factory->status),
                       factory->decoder_verified ? "decoder verified" : "decoder not yet vehicle-verified");
        link_gtk_card_append_detail(card, "X400 factory signal", factory_status);
        link_gtk_card_append_note(card, factory->provenance);
    }
    link_gtk_card_append_note(card,
        "LINK prefers a verified Jaguar factory value when available, otherwise uses measured SAE PID 0x5E fuel rate with PID 0x0D vehicle speed. Estimates are never presented as measured data.");
    gtk_box_append(GTK_BOX(body), card);
}

static void append_dashboard(GtkWidget *body, const JaglinkLinuxContext *context)
{
    static const char *keys[] = {
        "obd2.engine.rpm", "obd2.vehicle.speed", "obd2.engine.coolant",
        "obd2.engine.maf", "obd2.engine.throttle", "obd2.electrical.control_module_voltage"
    };
    GtkWidget *card = link_gtk_card_new("AT-A-GLANCE", "Jaguar powertrain dashboard");
    size_t index;
    link_gtk_card_append_status(card,
        context->diagnostic_ready ? "LIVE SAMPLES" : diagnostic_text(context),
        context->diagnostic_ready ? "state-success" : "state-warning");
    for (index = 0U; index < sizeof(keys) / sizeof(keys[0]); ++index) {
        const JaglinkParameterDefinition *definition = jaglink_parameter_obd2_definition_for_stable_key(keys[index]);
        char value[96];
        if (definition == NULL) continue;
        if (context->sample_valid[definition->key.identifier])
            format_sample(&context->samples[definition->key.identifier], value, sizeof(value));
        else
            (void)snprintf(value, sizeof(value), "Waiting");
        link_gtk_card_append_detail(card, definition->name, value);
    }
    gtk_box_append(GTK_BOX(body), card);
    append_fuel_economy(body, context);
}

static void append_generic_status(GtkWidget *body,
                                  const char *kicker,
                                  const char *title,
                                  const char *note,
                                  const JaglinkLinuxContext *context)
{
    GtkWidget *card = link_gtk_card_new(kicker, title);
    link_gtk_card_append_status(card, diagnostic_text(context),
                                context->diagnostic_ready ? "state-success" : "state-warning");
    link_gtk_card_append_note(card, note);
    gtk_box_append(GTK_BOX(body), card);
}

static void render_section(size_t section, GtkWidget *body, void *opaque)
{
    JaglinkLinuxContext *context = opaque;
    switch ((LinkWorkspaceSection)section) {
    case LINK_WORKSPACE_VEHICLE: append_vehicle(body, context); break;
    case LINK_WORKSPACE_MODULES: append_modules(body); break;
    case LINK_WORKSPACE_FAULTS: append_faults(body, context); break;
    case LINK_WORKSPACE_LIVE_DATA: append_parameters(body, false, context); break;
    case LINK_WORKSPACE_TABLE: append_parameters(body, true, context); break;
    case LINK_WORKSPACE_DASHBOARD: append_dashboard(body, context); break;
    case LINK_WORKSPACE_GRAPHS:
        append_generic_status(body, "INSTRUMENT TRACES", "Signal history",
                              "Time-series traces receive real LINK telemetry samples from the active Linux diagnostic flow.", context); break;
    case LINK_WORKSPACE_LOG:
        append_generic_status(body, "SESSION RECORDER", "Diagnostic evidence",
                              "Raw requests, responses and telemetry use the shared evidence path; no synthetic vehicle data is displayed.", context); break;
    case LINK_WORKSPACE_SETTINGS: {
        GtkWidget *card = link_gtk_card_new("JAGLINK", "System identity");
        link_gtk_card_append_detail(card, "Version", jaglink_version());
        link_gtk_card_append_detail(card, "Product", "Jaguar X-Type X400 diagnostics");
        link_gtk_card_append_detail(card, "Portable core", jaglink_self_check() ? "Validated" : "Invalid metadata");
        link_gtk_card_append_detail(card, "Linux transport", "LINK serial ELM327 provider");
        link_gtk_card_append_detail(card, "Linux live flow", "Automatic SAE PID + DTC + live polling");
        link_gtk_card_append_detail(card, "Jaguar factory layer", "X400 routes + module-specific DTC catalogue");
        link_gtk_card_append_detail(card, "Fuel economy", "Factory-priority + SAE measured fallback");
        gtk_box_append(GTK_BOX(body), card);
        break;
    }
    case LINK_WORKSPACE_SECTION_COUNT: break;
    }
}

static void show_about(GtkWindow *window, void *context)
{
    (void)context;
    jaglink_linux_show_about(window);
}

static void connection_changed(LinkTransport *transport,
                               bool connected,
                               const char *adapter_identity,
                               void *opaque)
{
    JaglinkLinuxContext *context = opaque;
    context->connected = connected;
    context->transport = *transport;
    (void)snprintf(context->adapter_identity, sizeof(context->adapter_identity), "%s",
                   connected && adapter_identity != NULL ? adapter_identity : "");
    if (connected)
        link_fuel_economy_reset_trip(&context->fuel_economy, monotonic_ms());
    else
        link_fuel_economy_init(&context->fuel_economy);
}

static void diagnostic_changed(const LinkDiagnosticFlow *flow,
                               const LinkDiagnosticFlowEvent *event,
                               bool active,
                               bool ready,
                               void *opaque)
{
    JaglinkLinuxContext *context = opaque;
    context->diagnostic_active = active;
    context->diagnostic_ready = ready;
    if (flow == NULL) {
        context->diagnostic_valid = false;
        memset(&context->diagnostic, 0, sizeof(context->diagnostic));
        memset(context->sample_valid, 0, sizeof(context->sample_valid));
        memset(context->samples, 0, sizeof(context->samples));
        link_fuel_economy_init(&context->fuel_economy);
        return;
    }
    context->diagnostic = *flow;
    context->diagnostic_valid = true;
    if (event != NULL && event->kind == LINK_DIAGNOSTIC_FLOW_EVENT_LIVE_SAMPLE) {
        const uint64_t now_ms = monotonic_ms();
        context->samples[event->sample.pid] = event->sample;
        context->sample_valid[event->sample.pid] = true;
        (void)link_fuel_economy_observe_obd2(
            &context->fuel_economy, &event->sample, now_ms);
        link_fuel_economy_tick(&context->fuel_economy, now_ms);
    }
}

int main(int argc, char **argv)
{
    JaglinkLinuxContext context = {0};
    LinkGtkShellDescriptor descriptor = {0};
    int status;

    link_fuel_economy_init(&context.fuel_economy);
    jaglink_linux_resources_register_resource();
    descriptor.app_id = "com.github.The-First-Infiltrator.Jaglink";
    descriptor.window_title = "JAGLINK · Jaguar X400 Diagnostics";
    descriptor.brand_name = "JAGLINK";
    descriptor.brand_subtitle = "JAGUAR X-TYPE · X400";
    descriptor.version = jaglink_version();
    descriptor.emblem_resource = "/com/github/The-First-Infiltrator/Jaglink/jaglink-emblem.png";
    descriptor.css = jaglink_css;
    descriptor.render_section = render_section;
    descriptor.show_about = show_about;
    descriptor.connection_changed = connection_changed;
    descriptor.diagnostic_changed = diagnostic_changed;
    descriptor.context = &context;
    status = link_gtk_shell_run(argc, argv, &descriptor);
    jaglink_linux_resources_unregister_resource();
    return status;
}
