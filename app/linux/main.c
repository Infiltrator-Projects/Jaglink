// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file main.c
 * @brief Native GTK4 product shell for the JAGLINK X400 face.
 *
 * Presentation remains deliberately thin: workspace identity and Jaguar
 * profile data come from the portable C API, while vehicle transport is kept
 * outside the GTK layer.  The canonical product emblem is embedded as a
 * GResource so development-tree and installed launches render identically.
 */
#include "about-dialog.h"
#include "jaglink/jaglink.h"
#include "jaglink/jaguar.h"

#include <gtk/gtk.h>
#include <stddef.h>

typedef struct JaglinkLinuxApp {
    GtkWindow *window;
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
    ".jag-about-button { margin: 10px 0 0 0; padding: 8px 12px; border: 1px solid rgba(183,154,98,0.45); border-radius: 10px; background: #0d3829; color: #eee8da; font-weight: 700; }"
    ".jag-about-button:hover { background: #0f4634; }"
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
    G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    gtk_css_provider_load_from_data(provider, jaglink_css, -1);
    G_GNUC_END_IGNORE_DEPRECATIONS
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

static GtkWidget *workspace_row(const JaglinkWorkspaceSectionDescriptor *descriptor)
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

static void update_content(JaglinkLinuxApp *app, JaglinkWorkspaceSection section)
{
    const JaglinkWorkspaceSectionDescriptor *descriptor =
        jaglink_workspace_section(section);
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
    update_content(app, (JaglinkWorkspaceSection)GPOINTER_TO_INT(value));
}

static void about_clicked(GtkButton *button, gpointer user_data)
{
    JaglinkLinuxApp *app = user_data;
    (void)button;
    jaglink_linux_show_about(app->window);
}

static GtkWidget *build_badge(void)
{
    GtkWidget *badge = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *emblem = gtk_image_new_from_resource(
        "/com/github/The-First-Infiltrator/Jaglink/jaglink-emblem.png");
    GtkWidget *name = gtk_label_new("JAGLINK");

    gtk_image_set_pixel_size(GTK_IMAGE(emblem), 62);
    gtk_widget_set_valign(emblem, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(name, "jag-brand");
    gtk_box_append(GTK_BOX(badge), emblem);
    gtk_box_append(GTK_BOX(badge), name);
    return badge;
}

static GtkWidget *build_sidebar(JaglinkLinuxApp *app)
{
    GtkWidget *sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    GtkWidget *brand = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget *badge = build_badge();
    GtkWidget *subtitle = gtk_label_new("X400  ·  JAGUAR DIAGNOSTICS");
    GtkWidget *version = gtk_label_new(jaglink_version());
    GtkWidget *list = gtk_list_box_new();
    GtkWidget *about_button = gtk_button_new_with_label("About JAGLINK");
    size_t index;

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
    gtk_widget_set_vexpand(list, TRUE);
    g_signal_connect(list, "row-selected", G_CALLBACK(row_selected), app);

    for (index = 0U; index < jaglink_workspace_section_count(); ++index) {
        const JaglinkWorkspaceSectionDescriptor *descriptor =
            jaglink_workspace_section_at(index);
        if (descriptor != NULL) {
            gtk_list_box_append(GTK_LIST_BOX(list), workspace_row(descriptor));
        }
    }

    gtk_widget_add_css_class(about_button, "jag-about-button");
    g_signal_connect(about_button, "clicked", G_CALLBACK(about_clicked), app);

    gtk_box_append(GTK_BOX(sidebar), list);
    gtk_box_append(GTK_BOX(sidebar), about_button);
    return sidebar;
}

static GtkWidget *build_content(JaglinkLinuxApp *app)
{
    const JaglinkJaguarVehicleProfile *profile = jaglink_jaguar_x400_profile();
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

    app->window = GTK_WINDOW(window);
    apply_jaglink_css();
    gtk_window_set_title(GTK_WINDOW(window), "JAGLINK · Jaguar X400 Diagnostics");
    gtk_window_set_default_size(GTK_WINDOW(window), 1120, 740);
    gtk_window_set_icon_name(GTK_WINDOW(window), "jaglink");

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
