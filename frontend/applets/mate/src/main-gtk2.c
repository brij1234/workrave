// Copyright (C) 2002 - 2011 Rob Caelers & Raymond Penners
// All rights reserved.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <mate-panel-applet.h>

#include <gio/gio.h>

#include "control.h"
#include "credits.h"
#include "nls.h"

#include "MenuEnums.hh"

typedef struct _WorkraveApplet
{
  MatePanelApplet *applet;
  GtkActionGroup *action_group;
  WorkraveTimerboxControl *timerbox_control;
  GtkImage *image;
  gboolean alive;
  int inhibit;
} WorkraveApplet;

static void dbus_call_finish(GDBusProxy *proxy, GAsyncResult *res, gpointer user_data);

struct Menuitems
{
  enum MenuCommand id;
  gboolean autostart;
  gboolean visible_when_not_running;
  char *action;
  gboolean inhibit;
};

static struct Menuitems menu_data[] =
  {
    { MENU_COMMAND_OPEN,                  TRUE,  TRUE,  "Open",          },
    { MENU_COMMAND_PREFERENCES,           FALSE, FALSE, "Preferences",   },
    { MENU_COMMAND_EXERCISES,             FALSE, FALSE, "Exercises",     },
    { MENU_COMMAND_REST_BREAK,            FALSE, FALSE, "Restbreak",     },
    { MENU_COMMAND_MODE_NORMAL,           FALSE, FALSE, "Normal",        },
    { MENU_COMMAND_MODE_QUIET,            FALSE, FALSE, "Quiet",         },
    { MENU_COMMAND_MODE_SUSPENDED,        FALSE, FALSE, "Suspended",     },
    { MENU_COMMAND_NETWORK_CONNECT,       FALSE, FALSE, "Join",          },
    { MENU_COMMAND_NETWORK_DISCONNECT,    FALSE, FALSE, "Disconnect",    },
    { MENU_COMMAND_NETWORK_LOG,           FALSE, FALSE, "ShowLog",       },
    { MENU_COMMAND_NETWORK_RECONNECT,     FALSE, FALSE, "Reconnect",     },
    { MENU_COMMAND_STATISTICS,            FALSE, FALSE, "Statistics",    },
    { MENU_COMMAND_ABOUT,                 FALSE, TRUE,  "About",         },
    { MENU_COMMAND_MODE_READING,          FALSE, FALSE, "ReadingMode",   },
    { MENU_COMMAND_QUIT,                  FALSE, FALSE, "Quit",          }
  };

int
lookup_menu_index_by_id(enum MenuCommand id)
{
  for (int i = 0; i < sizeof(menu_data)/sizeof(struct Menuitems); i++)
    {
      if (menu_data[i].id == id)
        {
          return i;
        }
    }

  return -1;
}

int
lookup_menu_index_by_action(const char *action)
{
  for (int i = 0; i < sizeof(menu_data)/sizeof(struct Menuitems); i++)
    {
      if (g_strcmp0(menu_data[i].action, action) == 0)
        {
          return i;
        }
    }

  return -1;
}

void on_alive_changed(gpointer instance, gboolean alive, gpointer user_data)
{
  WorkraveApplet *applet = (WorkraveApplet *)(user_data);
  applet->alive = alive;

  if (!alive)
    {
      for (int i = 0; i < sizeof(menu_data)/sizeof(struct Menuitems); i++)
        {
          GtkAction *action = gtk_action_group_get_action(applet->action_group, menu_data[i].action);
          gtk_action_set_visible(action, menu_data[i].visible_when_not_running);
        }
    }
}

void on_menu_changed(gpointer instance, GVariant *parameters, gpointer user_data)
{
  WorkraveApplet *applet = (WorkraveApplet *)user_data;
  applet->inhibit++;

  GVariantIter *iter;
  g_variant_get (parameters, "(a(sii))", &iter);

  char *text;
  int id;
  int flags;

  gboolean visible[sizeof(menu_data)/sizeof(struct Menuitems)];
  for (int i = 0; i < sizeof(menu_data)/sizeof(struct Menuitems); i++)
    {
      visible[i] = menu_data[i].visible_when_not_running;
    }

  while (g_variant_iter_loop(iter, "(sii)", &text, &id, &flags))
    {
      int index = lookup_menu_index_by_id((enum MenuCommand)id);
      if (index == -1)
        {
          continue;
        }

      GtkAction *action = gtk_action_group_get_action(applet->action_group, menu_data[index].action);

      if (flags & MENU_ITEM_FLAG_SUBMENU_END ||
          flags & MENU_ITEM_FLAG_SUBMENU_BEGIN)
        {
          continue;
        }

      visible[index] = TRUE;

      if (GTK_IS_TOGGLE_ACTION(action))
        {
          GtkToggleAction *toggle = GTK_TOGGLE_ACTION(action);
          gtk_toggle_action_set_active(toggle, flags & MENU_ITEM_FLAG_ACTIVE);
        }
    }

  g_variant_iter_free (iter);

  for (int i = 0; i < sizeof(menu_data)/sizeof(struct Menuitems); i++)
    {
      GtkAction *action = gtk_action_group_get_action(applet->action_group, menu_data[i].action);
      gtk_action_set_visible(action, visible[i]);
    }
  applet->inhibit--;
}


static void
dbus_call_finish(GDBusProxy *proxy, GAsyncResult *res, gpointer user_data)
{
  GError *error = NULL;
  GVariant *result;

  result = g_dbus_proxy_call_finish(proxy, res, &error);
  if (error != NULL)
    {
      g_warning("DBUS Failed: %s", error ? error->message : "");
      g_error_free(error);
    }

  if (result != NULL)
    {
      g_variant_unref(result);
    }
}

static void
on_menu_about(WorkraveApplet *applet)
{
  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(WORKRAVE_PKGDATADIR "/images/workrave.png", NULL);
  GtkAboutDialog *about = GTK_ABOUT_DIALOG(gtk_about_dialog_new());

  gtk_container_set_border_width(GTK_CONTAINER(about), 5);

  gtk_show_about_dialog(NULL,
                        "name", "Workrave",
#ifdef GIT_VERSION
                        "version", PACKAGE_VERSION "\n(" GIT_VERSION ")",
#else
                        "version", PACKAGE_VERSION,
#endif
                        "copyright", workrave_copyright,
                        "website", "http://www.workrave.org",
                        "website_label", "www.workrave.org",
                        "comments", _("This program assists in the prevention and recovery"
                                      " of Repetitive Strain Injury (RSI)."),
                        "translator-credits", workrave_translators,
                        "authors", workrave_authors,
                        "logo", pixbuf,
                        NULL);
  g_object_unref(pixbuf);
}

static void
on_menu_command(GtkAction *action, WorkraveApplet *applet)
{
  if (applet->inhibit > 0)
    {
      return;
    }

  int index = lookup_menu_index_by_action(gtk_action_get_name(action));
  if (index == -1)
    {
      return;
    }

  switch(menu_data[index].id)
    {
    case MENU_COMMAND_ABOUT:
      on_menu_about(applet);
      break;

    default:
      {
        GDBusProxy *proxy = workrave_timerbox_control_get_applet_proxy(applet->timerbox_control);
        if (proxy != NULL)
          {
            g_dbus_proxy_call(proxy,
                              "Command",
                              g_variant_new("(i)", menu_data[index].id),
                              menu_data[index].autostart ? G_DBUS_CALL_FLAGS_NONE : G_DBUS_CALL_FLAGS_NO_AUTO_START,
                              -1,
                              NULL,
                              (GAsyncReadyCallback) dbus_call_finish,
                              applet);
          }
      }
      break;
    }
}


static void
on_menu_radio_changed(GtkRadioAction *action, GtkRadioAction *current, WorkraveApplet *applet)
{
  on_menu_command(current, applet);
}

static const GtkActionEntry menu_actions [] = {
  { "Open", GTK_STOCK_OPEN, N_("_Open"),
    NULL, NULL,
    G_CALLBACK(on_menu_command) },

  { "Statistics", NULL, N_("_Statistics"),
    NULL, NULL,
    G_CALLBACK(on_menu_command) },

  { "Preferences", GTK_STOCK_PREFERENCES, N_("_Preferences"),
    NULL, NULL,
    G_CALLBACK(on_menu_command) },

  { "Exercises", NULL, N_("_Exercises"),
    NULL, NULL,
    G_CALLBACK(on_menu_command) },

  { "Restbreak", NULL, N_("_Restbreak"),
    NULL, NULL,
    G_CALLBACK(on_menu_command) },

  { "Mode", NULL, N_("_Mode"),
    NULL, NULL,
    NULL },

  { "Network", NULL, N_("_Network"),
    NULL, NULL,
    NULL },

  { "Join", NULL, N_("_Join"),
    NULL, NULL,
    G_CALLBACK(on_menu_command) },

  { "Disconnect", NULL, N_("_Disconnect"),
    NULL, NULL,
    G_CALLBACK(on_menu_command) },

  { "Reconnect", NULL, N_("_Reconnect"),
    NULL, NULL,
    G_CALLBACK(on_menu_command) },

  { "About", GTK_STOCK_ABOUT, N_("_About"),
    NULL, NULL,
    G_CALLBACK(on_menu_about) },

  { "Quit", GTK_STOCK_QUIT, N_("_Quit"),
    NULL, NULL,
    G_CALLBACK(on_menu_command) },
};


static const GtkToggleActionEntry toggle_actions[] = {
    { "ShowLog", NULL, N_("Show log"),
      NULL, NULL,
      G_CALLBACK(on_menu_command), FALSE },

    { "ReadingMode", NULL, N_("Reading mode"),
      NULL, NULL,
      G_CALLBACK(on_menu_command), FALSE },
};


static const GtkRadioActionEntry mode_actions[] = {
    { "Normal", NULL, N_("Normal"),
      NULL, NULL,
      0 },

    { "Suspended", NULL, N_("Suspended"),
      NULL, NULL,
      1 },

    { "Quiet", NULL, N_("Quiet"),
      NULL, NULL,
      2 },
};

static void workrave_applet_fill(WorkraveApplet *applet)
{
  mate_panel_applet_set_flags(applet->applet,
                              MATE_PANEL_APPLET_HAS_HANDLE);
  mate_panel_applet_set_background_widget(applet->applet, GTK_WIDGET(applet->applet));

  applet->timerbox_control = g_object_new(WORKRAVE_TIMERBOX_CONTROL_TYPE, NULL);
  applet->image = workrave_timerbox_control_get_image(applet->timerbox_control);
  g_signal_connect(G_OBJECT(applet->timerbox_control), "menu-changed", G_CALLBACK(on_menu_changed),  applet);
  g_signal_connect(G_OBJECT(applet->timerbox_control), "alive-changed", G_CALLBACK(on_alive_changed),  applet);

  workrave_timerbox_control_set_tray_icon_visible_when_not_running(applet->timerbox_control, TRUE);
  workrave_timerbox_control_set_tray_icon_mode(applet->timerbox_control, WORKRAVE_TIMERBOX_CONTROL_TRAY_ICON_MODE_ALWAYS);

  applet->action_group = gtk_action_group_new("WorkraveAppletActions");
  gtk_action_group_set_translation_domain(applet->action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_actions(applet->action_group,
                               menu_actions,
                               G_N_ELEMENTS (menu_actions),
                               applet);
  gtk_action_group_add_toggle_actions(applet->action_group,
                                      toggle_actions,
                                      G_N_ELEMENTS (toggle_actions),
                                      applet);
  gtk_action_group_add_radio_actions (applet->action_group,
                                      mode_actions,
                                      G_N_ELEMENTS(mode_actions),
                                      0,
                                      G_CALLBACK(on_menu_radio_changed),
                                      applet);

	gchar *ui_path = g_build_filename(WORKRAVE_MENU_UI_DIR, "workrave-menu.xml", NULL);
	mate_panel_applet_setup_menu_from_file(applet->applet, ui_path, applet->action_group);
	g_free(ui_path);

  gtk_container_add(GTK_CONTAINER(applet->applet), GTK_WIDGET(applet->image));
  gtk_widget_show_all(GTK_WIDGET(applet->applet));
}

static gboolean workrave_applet_factory(MatePanelApplet *applet, const gchar *iid, gpointer data)
{
  gboolean retval = FALSE;

  if (g_strcmp0(iid, "WorkraveApplet") == 0)
    {
      WorkraveApplet *workrave_applet;
      workrave_applet = g_new0(WorkraveApplet, 1);
      workrave_applet->applet = applet;
      workrave_applet->action_group= NULL;
      workrave_applet->timerbox_control = NULL;
      workrave_applet->image = NULL;
      workrave_applet->alive = FALSE;
      workrave_applet->inhibit = 0;
      workrave_applet_fill(workrave_applet);
      retval = TRUE;
    }

    return retval;
}

MATE_PANEL_APPLET_OUT_PROCESS_FACTORY(
    "WorkraveAppletFactory",
    PANEL_TYPE_APPLET,
    "WorkraveApplet",
    workrave_applet_factory,
    NULL);
