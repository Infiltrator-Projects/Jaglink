// SPDX-License-Identifier: GPL-3.0-or-later
#include "about-dialog.h"
#include "jaglink/jaglink.h"
#include "jaglink/jaguar.h"
#include "jaglink/parameter.h"
#include "link-gtk-shell.h"
#include "link-gtk-widgets.h"
#include "link/workspace.h"

#include <gtk/gtk.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

void jaglink_linux_resources_register_resource(void);
void jaglink_linux_resources_unregister_resource(void);

typedef struct JaglinkLinuxContext {
    bool connected;
    char adapter_identity[160];
    LinkTransport transport;
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

static const char *connection_text(const JaglinkLinuxContext *context)
{
    return context->connected ? "LINKED · ELM327 VERIFIED" : "NOT LINKED";
}

static void append_vehicle(GtkWidget *body, JaglinkLinuxContext *context)
{
    const JaglinkJaguarVehicleProfile *profile = jaglink_jaguar_x400_profile();
    GtkWidget *identity = link_gtk_card_new("VEHICLE PROFILE", "Jaguar X-Type X400");
    GtkWidget *connection = link_gtk_card_new("CONNECTION", "Linux diagnostic link");
    char years[32];

    if (profile != NULL)
        (void)snprintf(years, sizeof(years), "%u–%u",
                       (unsigned int)profile->first_model_year,
                       (unsigned int)profile->last_model_year);
    else
        (void)snprintf(years, sizeof(years), "Unavailable");

    link_gtk_card_append_detail(identity, "Platform", profile != NULL ? profile->platform_code : "Unavailable");
    link_gtk_card_append_detail(identity, "Family", profile != NULL ? profile->platform_family : "Unavailable");
    link_gtk_card_append_detail(identity, "Profile", profile != NULL ? profile->display_name : "Unavailable");
    link_gtk_card_append_detail(identity, "Model years", years);
    if (profile != NULL) {
        char networks[32];
        (void)snprintf(networks, sizeof(networks), "%zu defined networks", profile->network_count);
        link_gtk_card_append_detail(identity, "Network map", networks);
    }

    link_gtk_card_append_status(connection, connection_text(context),
                                context->connected ? "state-success" : "state-warning");
    link_gtk_card_append_detail(connection, "Adapter",
                                context->connected && context->adapter_identity[0] != '\0'
                                    ? context->adapter_identity : "Select an adapter above and press LINK UP");
    link_gtk_card_append_note(connection,
        "The shared LINK Linux transport opens the selected serial adapter and verifies its ELM327 identity before JAGLINK reports a live link.");
    gtk_box_append(GTK_BOX(body), identity);
    gtk_box_append(GTK_BOX(body), connection);
}

static void append_modules(GtkWidget *body)
{
    const JaglinkJaguarVehicleProfile *profile = jaglink_jaguar_x400_profile();
    GtkWidget *card = link_gtk_card_new("X400 NETWORK TOPOLOGY", "Diagnostic networks and module paths");
    size_t index;
    if (profile == NULL || profile->network_count == 0U) {
        link_gtk_card_append_status(card, "NO NETWORK DEFINITIONS", "state-warning");
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
            link_gtk_card_append_detail(card, network->name, detail);
        }
    }
    link_gtk_card_append_note(card,
        "JAGLINK Discover will populate ECU identities beneath these network paths as Jaguar-specific module evidence is added.");
    gtk_box_append(GTK_BOX(body), card);
}

static void append_faults(GtkWidget *body, const JaglinkLinuxContext *context)
{
    GtkWidget *standard = link_gtk_card_new("STANDARD OBD-II", "Stored, pending and permanent faults");
    GtkWidget *jaguar = link_gtk_card_new("JAGUAR MODULES", "Manufacturer fault inventory");
    const char *status = context->connected ? "LINK READY · SCAN NOT STARTED" : "NOT SCANNED · LINK OFFLINE";
    link_gtk_card_append_status(standard, status, context->connected ? "state-success" : "state-warning");
    link_gtk_card_append_note(standard,
        "Standards-based powertrain DTC decoding is provided by LINK and remains read-only.");
    link_gtk_card_append_status(jaguar, status, context->connected ? "state-success" : "state-warning");
    link_gtk_card_append_note(jaguar,
        "Jaguar-specific fault acquisition is enabled only as verified module addresses and safe requests are added to the X400 profile.");
    gtk_box_append(GTK_BOX(body), standard);
    gtk_box_append(GTK_BOX(body), jaguar);
}

static void append_parameters(GtkWidget *body, bool compact)
{
    GtkWidget *card = link_gtk_card_new(compact ? "PARAMETER TABLE" : "LIVE DATA CATALOGUE",
                                        compact ? "Standard OBD-II definitions" : "Available shared diagnostic parameters");
    size_t count = jaglink_parameter_obd2_definition_count();
    size_t index;
    for (index = 0U; index < count; ++index) {
        const JaglinkParameterDefinition *definition = jaglink_parameter_obd2_definition_at(index);
        char key[48];
        if (definition == NULL) continue;
        (void)snprintf(key, sizeof(key), "PID 0x%02X · %s",
                       (unsigned int)definition->key.identifier, definition->short_name);
        link_gtk_card_append_detail(card, compact ? key : definition->name,
                                    compact ? definition->name : key);
    }
    gtk_box_append(GTK_BOX(body), card);
}

static void append_dashboard(GtkWidget *body, const JaglinkLinuxContext *context)
{
    static const char *keys[] = {
        "obd2.engine.rpm", "obd2.vehicle.speed", "obd2.engine.coolant",
        "obd2.intake.maf", "obd2.throttle.position", "obd2.control.module.voltage"
    };
    GtkWidget *card = link_gtk_card_new("AT-A-GLANCE", "Jaguar powertrain dashboard");
    size_t index;
    link_gtk_card_append_status(card,
        context->connected ? "LINK READY · WAITING FOR LIVE SAMPLES" : "WAITING FOR LINK",
        context->connected ? "state-success" : "state-warning");
    for (index = 0U; index < sizeof(keys) / sizeof(keys[0]); ++index) {
        const JaglinkParameterDefinition *definition = jaglink_parameter_obd2_definition_for_stable_key(keys[index]);
        if (definition != NULL) link_gtk_card_append_detail(card, definition->name, "N/A");
    }
    gtk_box_append(GTK_BOX(body), card);
}

static void append_generic_status(GtkWidget *body,
                                  const char *kicker,
                                  const char *title,
                                  const char *note,
                                  const JaglinkLinuxContext *context)
{
    GtkWidget *card = link_gtk_card_new(kicker, title);
    link_gtk_card_append_status(card, context->connected ? "LINK READY" : "LINK OFFLINE",
                                context->connected ? "state-success" : "state-warning");
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
    case LINK_WORKSPACE_LIVE_DATA: append_parameters(body, false); break;
    case LINK_WORKSPACE_TABLE: append_parameters(body, true); break;
    case LINK_WORKSPACE_DASHBOARD: append_dashboard(body, context); break;
    case LINK_WORKSPACE_GRAPHS:
        append_generic_status(body, "INSTRUMENT TRACES", "Signal history",
                              "Time-series traces populate from real LINK telemetry samples.", context); break;
    case LINK_WORKSPACE_LOG:
        append_generic_status(body, "SESSION RECORDER", "Diagnostic evidence",
                              "Raw requests, responses and telemetry are preserved by the shared evidence pipeline.", context); break;
    case LINK_WORKSPACE_SETTINGS: {
        GtkWidget *card = link_gtk_card_new("JAGLINK", "System identity");
        link_gtk_card_append_detail(card, "Version", jaglink_version());
        link_gtk_card_append_detail(card, "Product", "Jaguar X-Type X400 diagnostics");
        link_gtk_card_append_detail(card, "Portable core", jaglink_self_check() ? "Validated" : "Invalid metadata");
        link_gtk_card_append_detail(card, "Linux transport", "LINK serial ELM327 provider");
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
}

int main(int argc, char **argv)
{
    JaglinkLinuxContext context = {0};
    LinkGtkShellDescriptor descriptor = {0};
    int status;

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
    descriptor.context = &context;
    status = link_gtk_shell_run(argc, argv, &descriptor);
    jaglink_linux_resources_unregister_resource();
    return status;
}
