// SPDX-License-Identifier: GPL-3.0-or-later
#include "mblink/mblink.h"
#include "mblink/jaguar.h"

#include <gtk/gtk.h>
#include <stddef.h>

typedef struct JaglinkLinuxApp {
    GtkWidget *content_title;
    GtkWidget *content_summary;
} JaglinkLinuxApp;

static const char *jaglink_css =
    "window { background: #061a13; color: #eee8da; }"
    ".jag-sidebar { background: #08251b; border-right: 1px solid rgba(183,154,98,0.45); }"
    ".jag-brand { color: #eee8da; font-family: serif; font-size: 30px; font-weight: 700; letter-spacing: 5px; }"
    ".jag-subtitle { color: #b79a62; font-size: 12px; font-weight: 700; letter-spacing: 1px; }"
    ".jag-version { color: #bfc5c8; font-size: 11px; }"
    ".jag-panel { background: #0d3829; border: 1px solid rgba(183,154,98,0.42); border-radius: 16px; padding: 20px; }"
    ".jag-title { color: #eee8da; font-family: serif; font-size: 30px; font-weight: 700; }"
    ".jag-summary { color: #c8c2b3; font-size: 15px; }"
    ".jag-accent { color: #b79a62; font-weight: 700; }"
    "list { background: transparent; }"
    "row { background: transparent; border-radius: 10px; margin: 2px 0; }"
    "row:hover { background: rgba(15,59,46,0.65); }"
    "row:selected { background: #0f3b2e; border: 1px solid rgba(183,154,98,0.55); }"
    "row:selected label { color: #eee8da; }"
    ".heading { color: #eee8da; font-weight: 700; }"
    ".dim-label { color: #9fa9a2; }";

static void set_margins(GtkWidget *widget, int margin)
{
    gtk_widget_set_margin_top(widget, margin);
    gtk_widget_set_margin_bottom(widget, margin);
    gtk_widget_set_margin_start(widget, margin);
    gtk_widget_set_margin_end(widget, margin);
}

static void apply_jaglink_css(void)
{
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, jaglink_css, -1);
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

static GtkWidget *workspace_row(const MblinkWorkspaceSectionDescriptor *descriptor)
{
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
    GtkWidget *title = gtk_label_new(descriptor->title);
    GtkWidget *summary = gtk_label_new(descriptor->summary);

    gtk_label_set_xalign(GTK_LABEL(title), 0.0F);
    gtk_label_set_xalign(GTK_LABEL(summary), 0.0F);
    gtk_label_set_wrap(GTK_LABEL(summary), TRUE);
    gtk_widget_add_css_class(title, "heading");
    gtk_widget_add_css_class(summary, "dim-label");
    set_margins(box, 11);

    gtk_box_append(GTK_BOX(box), title);
    gtk_box_append(GTK_BOX(box), summary);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), box);
    g_object_set_data(G_OBJECT(row), "jaglink-section",
                      GINT_TO_POINTER((int)descriptor->section));
    return row;
}

static void update_content(JaglinkLinuxApp *app, MblinkWorkspaceSection section)
{
    const MblinkWorkspaceSectionDescriptor *descriptor =
        mblink_workspace_section(section);
    if (descriptor == NULL) {
        return;
    }
    gtk_label_set_text(GTK_LABEL(app->content_title), descriptor->title);
    gtk_label_set_text(GTK_LABEL(app->content_summary), descriptor->summary);
}

static void row_selected(GtkListBox *list_box,
                         GtkListBoxRow *row,
                         gpointer user_data)
{
    JaglinkLinuxApp *app = user_data;
    gpointer value;
    (void)list_box;
    if (row == NULL) {
        return;
    }
    value = g_object_get_data(G_OBJECT(row), "jaglink-section");
    update_content(app, (MblinkWorkspaceSection)GPOINTER_TO_INT(value));
}

static GtkWidget *build_badge(void)
{
    GtkWidget *badge = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *mark = gtk_label_new("◆");
    GtkWidget *name = gtk_label_new("JAGLINK");

    gtk_widget_add_css_class(mark, "jag-accent");
    gtk_widget_add_css_class(name, "jag-brand");
    gtk_box_append(GTK_BOX(badge), mark);
    gtk_box_append(GTK_BOX(badge), name);
    return badge;
}

static GtkWidget *build_sidebar(JaglinkLinuxApp *app)
{
    GtkWidget *sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    GtkWidget *brand = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget *badge = build_badge();
    GtkWidget *subtitle = gtk_label_new("X400  ·  JAGUAR DIAGNOSTICS");
    GtkWidget *version = gtk_label_new(mblink_version());
    GtkWidget *list = gtk_list_box_new();
    size_t index;

    (void)app;
    gtk_widget_set_size_request(sidebar, 320, -1);
    gtk_widget_add_css_class(sidebar, "jag-sidebar");
    set_margins(sidebar, 16);

    gtk_label_set_xalign(GTK_LABEL(subtitle), 0.0F);
    gtk_label_set_xalign(GTK_LABEL(version), 0.0F);
    gtk_widget_add_css_class(subtitle, "jag-subtitle");
    gtk_widget_add_css_class(version, "jag-version");

    gtk_box_append(GTK_BOX(brand), badge);
    gtk_box_append(GTK_BOX(brand), subtitle);
    gtk_box_append(GTK_BOX(brand), version);
    gtk_box_append(GTK_BOX(sidebar), brand);

    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list), GTK_SELECTION_SINGLE);
    g_signal_connect(list, "row-selected", G_CALLBACK(row_selected), app);

    for (index = 0U; index < mblink_workspace_section_count(); ++index) {
        const MblinkWorkspaceSectionDescriptor *descriptor =
            mblink_workspace_section_at(index);
        if (descriptor != NULL) {
            gtk_list_box_append(GTK_LIST_BOX(list), workspace_row(descriptor));
        }
    }

    gtk_box_append(GTK_BOX(sidebar), list);
    return sidebar;
}

static GtkWidget *build_content(JaglinkLinuxApp *app)
{
    const MblinkJaguarVehicleProfile *profile = mblink_jaguar_x400_profile();
    GtkWidget *outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    GtkWidget *eyebrow = gtk_label_new("JAGUAR X-TYPE  ·  X400");
    GtkWidget *notice = gtk_label_new(
        profile != NULL ? profile->display_name : "Jaguar X-Type X400");

    gtk_widget_add_css_class(content, "jag-panel");
    gtk_widget_add_css_class(eyebrow, "jag-accent");
    gtk_label_set_xalign(GTK_LABEL(eyebrow), 0.0F);

    app->content_title = gtk_label_new("Vehicle");
    app->content_summary = gtk_label_new(
        "X400 diagnostic cockpit: CAN, SCP, ISO 9141 and D2B network provenance, "
        "fault memory, live data and session logging in one Jaguar-focused workspace.");

    gtk_label_set_xalign(GTK_LABEL(app->content_title), 0.0F);
    gtk_label_set_xalign(GTK_LABEL(app->content_summary), 0.0F);
    gtk_label_set_xalign(GTK_LABEL(notice), 0.0F);
    gtk_label_set_wrap(GTK_LABEL(app->content_summary), TRUE);
    gtk_label_set_wrap(GTK_LABEL(notice), TRUE);
    gtk_widget_add_css_class(app->content_title, "jag-title");
    gtk_widget_add_css_class(app->content_summary, "jag-summary");
    gtk_widget_add_css_class(notice, "dim-label");

    gtk_box_append(GTK_BOX(content), eyebrow);
    gtk_box_append(GTK_BOX(content), app->content_title);
    gtk_box_append(GTK_BOX(content), app->content_summary);
    gtk_box_append(GTK_BOX(content), notice);

    set_margins(outer, 28);
    gtk_box_append(GTK_BOX(outer), content);
    return outer;
}

static void activate(GtkApplication *application, gpointer user_data)
{
    JaglinkLinuxApp *app = user_data;
    GtkWidget *window = gtk_application_window_new(application);
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *sidebar;
    GtkWidget *content;

    apply_jaglink_css();
    gtk_window_set_title(GTK_WINDOW(window), "JAGLINK · Jaguar X400 Diagnostics");
    gtk_window_set_default_size(GTK_WINDOW(window), 1120, 740);

    sidebar = build_sidebar(app);
    content = build_content(app);
    gtk_widget_set_hexpand(content, TRUE);
    gtk_widget_set_vexpand(content, TRUE);

    gtk_box_append(GTK_BOX(root), sidebar);
    gtk_box_append(GTK_BOX(root), content);
    gtk_window_set_child(GTK_WINDOW(window), root);
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv)
{
    GtkApplication *application;
    JaglinkLinuxApp app = { 0 };
    int status;

    application = gtk_application_new(
        "com.github.The-First-Infiltrator.Jaglink",
        G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(application, "activate", G_CALLBACK(activate), &app);
    status = g_application_run(G_APPLICATION(application), argc, argv);
    g_object_unref(application);
    return status;
}
