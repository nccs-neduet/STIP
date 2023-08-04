/*
  License:

  Copyright (c) 2003-2006 ossim.net
  Copyright (c) 2007-2013 AlienVault
  All rights reserved.

  This package is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 dated June, 1991.
  You may not use, modify or distribute this program under any other version
  of the GNU General Public License.

  This package is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this package; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
  MA  02110-1301  USA


  On Debian GNU/Linux systems, the complete text of the GNU General
  Public License can be found in `/usr/share/common-licenses/GPL-2'.

  Otherwise you can read it here: http://www.gnu.org/licenses/gpl-2.0.txt
*/

#include "config.h"

#include "sim-event.h"

#include <pcre.h>
#include <time.h>
#include <math.h>
#include <string.h> //strlen()
#include <json-glib/json-glib.h>

#include "sim-util.h"
#include "sim-organizer.h"
#include "sim-idm.h"
#include "sim-policy.h"
#include "os-sim.h"
#include "sim-debug.h"

extern SimMain  ossim;
static gchar *sql_text_fields = NULL;

static void       sim_event_init (SimEvent *event);
static SimEvent * _sim_event_copy (SimEvent *event);
static void       _sim_event_free (SimEvent *event);
static void       sim_event_idm_get_insert_clause_values_one (gpointer key, gpointer value, gpointer user_data);


/* Data for sim_event_idm_get_insert_clause_values */
typedef struct _SimIdmInsertAux
{
  GString       *gstr;
  const gchar   *event_id;
  GdaConnection *conn;
  gboolean       from_src;
} SimIdmInsertAux;

typedef struct _SimOtxDataIter
{
  SimUuid *uuid;
  GString *st;
  GdaConnection *conn;
}SimOtxDataIter;

/* GType Functions */

GType _sim_event_type = 0;

SIM_DEFINE_MINI_OBJECT_TYPE (SimEvent, sim_event);

void
_priv_sim_event_initialize (void)
{
  GString *st;
  gint i;

  _sim_event_type = sim_event_get_type ();

  st = g_string_new ("");
  for (i = 0; i < N_TEXT_FIELDS; i++)
  {
    g_string_append_printf (st, "%s%s", sim_text_field_get_name (i), (i != (N_TEXT_FIELDS - 1)) ? "," : "");
  }

  /* This is a leak, the string will never be freed */
  sql_text_fields = g_string_free (st, FALSE);
}

static void
sim_event_init (SimEvent *event)
{
  sim_mini_object_init (SIM_MINI_OBJECT_CAST (event), _sim_event_type);

  event->mini_object.copy = (SimMiniObjectCopyFunction) _sim_event_copy;
  event->mini_object.dispose = NULL;
  event->mini_object.free = (SimMiniObjectFreeFunction) _sim_event_free;
}

SimEvent *
sim_event_new (void)
{
  SimEvent *event;
  GString *st;
  gint i;

  event = g_slice_new0 (SimEvent);

  sim_event_init (event);

  st = g_string_new ("");
  for (i = 0; i < N_TEXT_FIELDS; i++)
  {
    g_string_append_printf (st, "%s%s", sim_text_field_get_name (i), (i != (N_TEXT_FIELDS - 1)) ? "," : "");
  }

  event->sql_text_fields = g_string_free (st, FALSE);
  ossim_debug ("sim_event_instance_init");

  event->signature = 0xdeadbeef;

  event->type = SIM_EVENT_TYPE_NONE;
  event->log = NULL;

  //The agent should always write a date/time field in the events, so it's not necesary to initialize time, time_str, diff_time and tzone
  //
  event->protocol = SIM_PROTOCOL_TYPE_NONE;
  event->condition = SIM_CONDITION_TYPE_NONE;

  event->asset_src = VOID_ASSET;
  event->asset_dst = VOID_ASSET;

  event->store_in_DB = TRUE; //we want to store everything by default
  event->can_delete= FALSE;  // It has to be processed by 2 threads before destruction

  event->is_correlated = FALSE;	//local mode
  event->correlation = EVENT_MATCH_NOTHING;
  event->belongs_to_alarm = FALSE;	//this is send across network

  event->policy= NULL; //DON'T release memory for this variable!!!! (the data inside belongs to Container)

  event->context = NULL;
  event->engine = NULL;
  event->otx_data = g_hash_table_new_full (g_str_hash, g_str_equal,
                        g_free, (GDestroyNotify)g_ptr_array_unref);
  return event;
}

SimEvent *
sim_event_new_from_type (SimEventType   type)
{
  SimEvent *event = NULL;

  event = sim_event_new ();
  event->type = type;

  // Assign a new unique ID.
  event->id = sim_uuid_new ();

  sim_event_set_context_and_engine (event, SIM_CONTEXT_DEFAULT);

  return event;
}

SimEvent *
sim_event_new_full (SimEventType  type,
                    SimUuid      *event_id,
                    SimUuid      *context_id)
{
  SimEvent *event;

  event = sim_event_new ();
  event->type = type;

  if (event_id)
    event->id = g_object_ref (event_id);
  else
    event->id = sim_uuid_new ();

  if (context_id)
    sim_event_set_context_and_engine (event, context_id);
  else
    sim_event_set_context_and_engine (event, SIM_CONTEXT_DEFAULT);

  return event;
}

SimEvent *
sim_event_light_new_from_dm (GdaDataModel *dm, gint row)
{
  SimEvent *event;
  const GValue *value;
  const GdaBinary *binary;
  const GdaTimestamp *timestamp;
  struct tm tm;

  g_return_val_if_fail (GDA_IS_DATA_MODEL (dm), NULL);

  event = sim_event_new ();

  event->id = sim_uuid_new ();

  value = gda_data_model_get_value_at (dm, 0, row, NULL);
  if (!gda_value_is_null (value))
  {
    binary = (GdaBinary *) gda_value_get_blob (value);
    event->src_ia = sim_inet_new_from_db_binary (binary->data, binary->binary_length);
  }

  value = gda_data_model_get_value_at (dm, 1, row, NULL);
  if (!gda_value_is_null (value))
  {
    binary = (GdaBinary *) gda_value_get_blob (value);
    event->dst_ia = sim_inet_new_from_db_binary (binary->data, binary->binary_length);
  }

  value = gda_data_model_get_value_at (dm, 2, row, NULL);
  if (!gda_value_is_null (value))
    event->src_id = sim_uuid_new_from_blob (gda_value_get_blob (value));

  value = gda_data_model_get_value_at (dm, 3, row, NULL);
  if (!gda_value_is_null (value))
    event->dst_id = sim_uuid_new_from_blob (gda_value_get_blob (value));

  value = gda_data_model_get_value_at (dm, 4, row, NULL);
  event->src_port = gda_value_is_null (value) ? 0 : g_value_get_int (value);

  value = gda_data_model_get_value_at (dm, 5, row, NULL);
  event->dst_port = gda_value_is_null (value) ? 0 : g_value_get_int (value);

  value = gda_data_model_get_value_at (dm, 6, row, NULL);
  if (!gda_value_is_null (value))
  {
    timestamp = gda_value_get_timestamp (value);
    tm.tm_year = timestamp->year - 1900;
    tm.tm_mon = timestamp->month - 1;
    tm.tm_mday = timestamp->day;
    tm.tm_hour = timestamp->hour - 1;
    tm.tm_min = timestamp->minute;
    tm.tm_mday = timestamp->day;
    tm.tm_sec = timestamp->second;
    tm.tm_wday = 0;
    tm.tm_yday = 0;
    tm.tm_isdst = 0;
    event->time = mktime (&tm);
  }

  value = gda_data_model_get_value_at (dm, 7, row, NULL);
  event->tzone = gda_value_is_null (value) ? 0.0 : g_value_get_float (value);

  return event;
}

SimEvent *
sim_event_ref (SimEvent *event)
{
  return (SimEvent *) sim_mini_object_ref (SIM_MINI_OBJECT_CAST (event));
}

void
sim_event_unref_null (SimEvent *event)
{
  if (event)
    sim_event_unref (event);
}

void
sim_event_unref (SimEvent *event)
{
  SimMiniObject * mini_object = SIM_MINI_OBJECT_CAST (event);

  sim_mini_object_unref (mini_object);
  return;
}

SimEvent *
sim_event_copy (SimEvent *event)
{
  return (SimEvent *) sim_mini_object_copy (SIM_MINI_OBJECT_CAST (event));
}

static SimEvent *
_sim_event_copy (SimEvent *event)
{
  SimEvent  *new_event;
  GHashTableIter iter;
  gpointer key, value;

  new_event = sim_event_new ();

  new_event->type = event->type;
  new_event->id = g_object_ref (event->id);

  new_event->time = event->time;
  new_event->time_str= g_strdup(event->time_str);
  new_event->tzone = event->tzone;

  (event->sensor) ? new_event->sensor = sim_inet_clone (event->sensor) : NULL;
  (event->sensor_id) ? new_event->sensor_id = g_object_ref (event->sensor_id) : NULL;
  (event->device) ? new_event->device = sim_inet_clone (event->device) : NULL;
  new_event->device_id = event->device_id;
  (event->interface) ? new_event->interface = g_strdup (event->interface) : NULL;

  new_event->plugin_id = event->plugin_id;
  new_event->plugin_sid = event->plugin_sid;

  new_event->protocol = event->protocol;

  if (event->src_id)
    new_event->src_id = g_object_ref (event->src_id);
  if (event->dst_id)
    new_event->dst_id = g_object_ref (event->dst_id);

  (event->src_ia) ? new_event->src_ia = sim_inet_clone (event->src_ia): NULL;
  (event->dst_ia) ? new_event->dst_ia = sim_inet_clone (event->dst_ia): NULL;
  (event->src_net) ? new_event->src_net = g_object_ref (event->src_net): NULL;
  (event->dst_net) ? new_event->dst_net = g_object_ref (event->dst_net): NULL;
  new_event->src_port = event->src_port;
  new_event->dst_port = event->dst_port;

  new_event->condition = event->condition;
  (event->value) ? new_event->value = g_strdup (event->value) : NULL;
  new_event->interval = event->interval;

  new_event->alarm = event->alarm;
  new_event->priority = event->priority;
  new_event->reliability = event->reliability;
  new_event->asset_src = event->asset_src;
  new_event->asset_dst = event->asset_dst;
  new_event->risk_c = event->risk_c;
  new_event->risk_a = event->risk_a;

  if (event->backlog_id)
    new_event->backlog_id = g_object_ref (event->backlog_id);

  if (event->role)
    new_event->role = sim_role_ref (event->role);

  if (event->log)
    new_event->log = g_string_new_len (event->log->str, event->log->len);

  (event->filename) ? new_event->filename = g_strdup (event->filename) : NULL;
  (event->username) ? new_event->username = g_strdup (event->username) : NULL;
  (event->password) ? new_event->password = g_strdup (event->password) : NULL;
  (event->userdata1) ? new_event->userdata1 = g_strdup (event->userdata1) : NULL;
  (event->userdata2) ? new_event->userdata2 = g_strdup (event->userdata2) : NULL;
  (event->userdata3) ? new_event->userdata3 = g_strdup (event->userdata3) : NULL;
  (event->userdata4) ? new_event->userdata4 = g_strdup (event->userdata4) : NULL;
  (event->userdata5) ? new_event->userdata5 = g_strdup (event->userdata5) : NULL;
  (event->userdata6) ? new_event->userdata6 = g_strdup (event->userdata6) : NULL;
  (event->userdata7) ? new_event->userdata7 = g_strdup (event->userdata7) : NULL;
  (event->userdata8) ? new_event->userdata8 = g_strdup (event->userdata8) : NULL;
  (event->userdata9) ? new_event->userdata9 = g_strdup (event->userdata9) : NULL;

  (event->userdata9) ? new_event->rulename = g_strdup (event->rulename) : NULL;

  if (event->src_username)
  {
    new_event->src_username = sim_command_idm_event_parse_username (event->src_username_raw);
    new_event->src_username_raw = g_strdup (new_event->src_username_raw);
  }

  if (event->dst_username)
  {
    new_event->dst_username = sim_command_idm_event_parse_username (event->dst_username_raw);
    new_event->dst_username_raw = g_strdup (new_event->dst_username_raw);
  }

  (event->src_hostname) ? new_event->src_hostname = g_strdup (event->src_hostname) : NULL;
  (event->dst_hostname) ? new_event->dst_hostname = g_strdup (event->dst_hostname) : NULL;
  (event->src_mac) ? new_event->src_mac = g_strdup (event->src_mac) : NULL;
  (event->dst_mac) ? new_event->dst_mac = g_strdup (event->dst_mac) : NULL;

  new_event->rep_act_src = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, NULL);
  g_hash_table_iter_init (&iter, event->rep_act_src);
  while (g_hash_table_iter_next (&iter, &key, &value)) 
    g_hash_table_insert(new_event->rep_act_src, key, value);

  new_event->rep_act_dst = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, NULL);
  g_hash_table_iter_init (&iter, event->rep_act_dst);
  while (g_hash_table_iter_next (&iter, &key, &value)) 
    g_hash_table_insert(new_event->rep_act_dst, key, value);

  new_event->rep_prio_src = event->rep_prio_src;
  new_event->rep_prio_dst = event->rep_prio_dst;
  new_event->rep_rel_src = event->rep_rel_src;
  new_event->rep_rel_dst = event->rep_rel_dst;

  new_event->tax_product = event->tax_product;
  new_event->tax_category = event->tax_category;
  new_event->tax_subcategory = event->tax_subcategory;

  (event->buffer) ? new_event->buffer = g_strdup (event->buffer) : NULL;
  event->is_stored = FALSE;

  if (event->binary_data)
    new_event->binary_data = g_strdup (event->binary_data);

  if (event->context)
    new_event->context = g_object_ref (event->context);

  if (event->engine)
    new_event->engine = g_object_ref (event->engine);
  /* Ref to events */
  if (new_event->otx_data)
    g_hash_table_unref (new_event->otx_data);
  new_event->otx_data = g_hash_table_ref (event->otx_data);

  return new_event;
}

static void
_sim_event_free (SimEvent *event)
{
  ossim_debug ("%s: Id %s, Device_id %u", __func__,
               sim_uuid_get_string (event->id), event->device_id);

  if (event->id)
    g_object_unref (event->id);

  if (event->src_ia)
    g_object_unref (event->src_ia);
  if (event->dst_ia)
    g_object_unref (event->dst_ia);

  if (event->src_net)
    g_object_unref (event->src_net);
  if (event->dst_net)
    g_object_unref (event->dst_net);

  if (event->src_id)
    g_object_unref (event->src_id);
  if (event->dst_id)
    g_object_unref (event->dst_id);

  if (event->sensor)
    g_object_unref (event->sensor);
  if (event->sensor_id)
    g_object_unref (event->sensor_id);
  if (event->device)
    g_object_unref (event->device);
  g_free (event->interface);
  g_free (event->server);
  if (event->servername)
    g_free (event->servername);

  g_free (event->sql_text_fields);
  g_free (event->value);
  g_free (event->data);

  if (event->role)
    sim_role_unref (event->role);
  if (event->policy)
    g_object_unref (event->policy);

  g_free (event->time_str);
  if (event->log)
    g_string_free (event->log, TRUE);
  g_free (event->alarm_stats);

  g_free (event->filename);    //no needed to check, g_free will just return if "filename" is NULL
  g_free (event->username);
  g_free (event->password);
  g_free (event->userdata1);
  g_free (event->userdata2);
  g_free (event->userdata3);
  g_free (event->userdata4);
  g_free (event->userdata5);
  g_free (event->userdata6);
  g_free (event->userdata7);
  g_free (event->userdata8);
  g_free (event->userdata9);

  g_free (event->rulename);

  if (event->src_username)
    g_hash_table_unref (event->src_username);
  g_free (event->src_username_raw);
  if (event->dst_username)
    g_hash_table_unref (event->dst_username);
  g_free (event->dst_username_raw);
  g_free (event->src_hostname);
  g_free (event->dst_hostname);
  g_free (event->src_mac);
  g_free (event->dst_mac);

  g_free (event->str_rep_act_src);
  if(event->rep_act_src)
    g_hash_table_unref (event->rep_act_src);
  g_free (event->str_rep_act_dst);
  if(event->rep_act_dst)
    g_hash_table_unref (event->rep_act_dst);

  g_free (event->buffer);

  if (event->groupalarmsha1)
    g_free (event->groupalarmsha1);

  if (event->binary_data)
    g_free (event->binary_data);

  if (event->context)
    g_object_unref (event->context);

  if (event->engine)
    g_object_unref (event->engine);
  if (event->otx_data)
    g_hash_table_unref (event->otx_data);

  g_slice_free (SimEvent, event);
}

/*
 *
 *
 *
 *
 */
SimEventType
sim_event_get_type_from_str (const gchar *str)
{
  g_return_val_if_fail (str, SIM_EVENT_TYPE_NONE);

  if (!g_ascii_strcasecmp (str, SIM_DETECTOR_CONST))
    return SIM_EVENT_TYPE_DETECTOR;
  else if (!g_ascii_strcasecmp (str, SIM_MONITOR_CONST))
    return SIM_EVENT_TYPE_MONITOR;
  else
    return SIM_EVENT_TYPE_NONE;
}

gchar*
sim_event_get_str_from_type (SimEventType type)
{
  if (type == SIM_EVENT_TYPE_DETECTOR)
    return (g_ascii_strdown (SIM_DETECTOR_CONST, strlen (SIM_DETECTOR_CONST)));
  else if (type == SIM_EVENT_TYPE_MONITOR)
    return (g_ascii_strdown (SIM_MONITOR_CONST, strlen (SIM_MONITOR_CONST)));
  else
    return NULL;
}

/*
 *
 *
 *
 *
 */
gchar *
sim_event_get_insert_clause (SimEvent *event)
{
  GString *query;
  gchar *header;
  gchar *values;

  g_return_val_if_fail (SIM_IS_EVENT (event), NULL);

  header = sim_event_get_insert_clause_header ();
  values = sim_event_get_insert_clause_values (event);

  query = g_string_new ("INSERT INTO event ");
  query = g_string_append (query, header);
  query = g_string_append (query, " VALUES ");
  query = g_string_append (query, values);

  g_free (header);
  g_free (values);

  return g_string_free (query, FALSE);
}

gchar *
sim_event_get_text_escape_fields_values (SimEvent *event)
{
  int i;
  gchar *e_fields[N_TEXT_FIELDS];
  gchar *fields[N_TEXT_FIELDS];
  GString * st;
  GdaConnection *conn;
  gchar *src_mac = NULL, *dst_mac = NULL;

  conn = sim_database_get_conn (ossim.dbossim);

  st = g_string_new ("");
  if (st == NULL)
    return NULL;

  if (event->src_mac)
    src_mac = sim_mac_to_db_string (event->src_mac);
  if (event->dst_mac)
    dst_mac = sim_mac_to_db_string (event->dst_mac);

  fields[SimTextFieldUsername] = event->username;
  fields[SimTextFieldPassword] = event->password;
  fields[SimTextFieldFilename] = event->filename;
  fields[SimTextFieldUserdata1]  = event->userdata1;
  fields[SimTextFieldUserdata2]  = event->userdata2;
  fields[SimTextFieldUserdata3]  = event->userdata3;
  fields[SimTextFieldUserdata4]  = event->userdata4;
  fields[SimTextFieldUserdata5]  = event->userdata5;
  fields[SimTextFieldUserdata6]  = event->userdata6;
  fields[SimTextFieldUserdata7]  = event->userdata7;
  fields[SimTextFieldUserdata8]  = event->userdata8;
  fields[SimTextFieldUserdata9]  = event->userdata9;
  fields[SimTextFieldRulename] = event->rulename;
  fields[SimTextFieldValue] = event->value;

  for (i = 0; i< N_TEXT_FIELDS; i++)
  {
    if (fields[i] != NULL)
    {
      e_fields[i] = sim_str_escape (fields[i], conn, 0);
      g_string_append_printf (st, "'%s'%s", e_fields[i], i != (N_TEXT_FIELDS-1) ? "," : "");
      g_free (e_fields[i]);
    }
    else
    {
      g_string_append_printf (st, "'%s'%s","", i != (N_TEXT_FIELDS-1) ? "," : "");
    }

  }

  g_free (src_mac);
  g_free (dst_mac);

  return g_string_free (st,FALSE);
}

gchar *
sim_event_get_insert_clause_values (SimEvent   *event)
{
  gchar  timebuff[TIMEBUF_SIZE];
	gchar *timestamp = timebuff;
  GString *query;
	gchar *values;
  gchar *e_rep_act_src = NULL;
  gchar *e_rep_act_dst = NULL;
  gchar *e_src_hostname = NULL;
  gchar *e_dst_hostname = NULL;
  gchar *src_mac = NULL, *dst_mac = NULL;
  GdaConnection *conn;
  gchar *tsfree = NULL;

  g_return_val_if_fail (SIM_IS_EVENT (event), NULL);

  conn = sim_database_get_conn (ossim.dbossim);

	values =  sim_event_get_text_escape_fields_values  (event);

  // If we already have the timestamp we use it.. else we calculate it
  if(event->time_str)
  {
    timestamp = tsfree =  sim_str_escape (event->time_str, conn, 0);
  }
	else
    sim_time_t_to_str (timestamp, event->time);

  if (event->str_rep_act_src)
    e_rep_act_src = sim_str_escape (event->str_rep_act_src, conn, 0);
  if (event->str_rep_act_dst)
    e_rep_act_dst = sim_str_escape (event->str_rep_act_dst, conn, 0);

  if (event->src_hostname)
    e_src_hostname = sim_str_escape (event->src_hostname, conn, 0);
  if (event->dst_hostname)
    e_dst_hostname = sim_str_escape (event->dst_hostname, conn, 0);

  if (event->src_mac)
    src_mac = sim_mac_to_db_string (event->src_mac);
  if (event->dst_mac)
    dst_mac = sim_mac_to_db_string (event->dst_mac);

  query = g_string_new ("");
  g_string_append_printf (query, "(%s", sim_uuid_get_db_string (event->id));
  g_string_append_printf (query, ",%s", sim_uuid_get_db_string (sim_context_get_id (event->context)));
  g_string_append_printf (query, ",'%s'", timestamp);
  g_string_append_printf (query, ",%f", event->tzone);
  g_string_append_printf (query, ",%s", sim_uuid_get_db_string (event->sensor_id));
  g_string_append_printf (query, ",'%s'", (event->interface) ? event->interface : "");
  g_string_append_printf (query, ",%d", event->type);
  g_string_append_printf (query, ",%d", event->plugin_id);
  g_string_append_printf (query, ",%d", event->plugin_sid);
  g_string_append_printf (query, ",%d", event->protocol);
  g_string_append_printf (query, ",%s", sim_inet_get_db_string (event->src_ia));
  g_string_append_printf (query, ",%s", sim_inet_get_db_string (event->dst_ia));
  g_string_append_printf (query, ",%s", (event->src_net) ? sim_uuid_get_db_string (sim_net_get_id (event->src_net)) : "NULL");
  g_string_append_printf (query, ",%s", (event->dst_net) ? sim_uuid_get_db_string (sim_net_get_id (event->dst_net)) : "NULL");
  g_string_append_printf (query, ",%d", event->src_port);
  g_string_append_printf (query, ",%d", event->dst_port);
  g_string_append_printf (query, ",%d", event->condition);
  g_string_append_printf (query, ",%d", event->interval);
  g_string_append_printf (query, ",%d", 0); //FIXME event->absolute
  g_string_append_printf (query, ",%d", event->priority);
  g_string_append_printf (query, ",%d", event->reliability);
  g_string_append_printf (query, ",%d", event->asset_src);
  g_string_append_printf (query, ",%d", event->asset_dst);
  g_string_append_printf (query, ",%d", (gint) event->risk_c);
  g_string_append_printf (query, ",%d", (gint) event->risk_a);
  g_string_append_printf (query, ",%d", event->alarm);
  g_string_append_printf (query, ",%s", values);
  g_string_append_printf (query, ",%u", event->rep_prio_src);
  g_string_append_printf (query, ",%u", event->rep_prio_dst);
  g_string_append_printf (query, ",%u", event->rep_rel_src);
  g_string_append_printf (query, ",%u", event->rep_rel_dst);
  g_string_append_printf (query, ",'%s'", (e_rep_act_src) ? e_rep_act_src : "");
  g_string_append_printf (query, ",'%s'", (e_rep_act_dst) ? e_rep_act_dst : "");
  g_string_append_printf (query, ",'%s'", (e_src_hostname) ? e_src_hostname : "");
  g_string_append_printf (query, ",'%s'", (e_dst_hostname) ? e_dst_hostname : "");
  g_string_append_printf (query, ",%s", (src_mac) ? src_mac : "NULL");
  g_string_append_printf (query, ",%s", (dst_mac) ? dst_mac : "NULL");
  g_string_append_printf (query, ",%s", (event->src_id) ? sim_uuid_get_db_string (event->src_id) : "NULL");
  g_string_append_printf (query, ",%s)", (event->dst_id) ? sim_uuid_get_db_string (event->dst_id) : "NULL");

  g_free (values);
  g_free (tsfree);

  return g_string_free (query, FALSE);
}

gchar *
sim_event_get_insert_clause_header (void)
{
  gchar *query;

	query = g_strdup_printf ("(id, "
                           "agent_ctx, "
                           "timestamp, "
                           "tzone, "
                           "sensor_id, "
                           "interface, "
                           "type, "
                           "plugin_id, "
                           "plugin_sid, "
                           "protocol, "
                           "src_ip, "
                           "dst_ip, "
                           "src_net, "
                           "dst_net, "
                           "src_port, "
                           "dst_port, "
                           "event_condition, "
                           "time_interval, "
                           "absolute, "
                           "priority, "
                           "reliability, "
                           "asset_src, "
                           "asset_dst, "
                           "risk_c, "
                           "risk_a, "
                           "alarm, "
                           "%s, "
                           "rep_prio_src, "
                           "rep_prio_dst, "
                           "rep_rel_src, "
                           "rep_rel_dst, "
                           "rep_act_src, "
                           "rep_act_dst, "
                           "src_hostname, "
                           "dst_hostname, "
                           "src_mac, "
                           "dst_mac, "
                           "src_host, "
                           "dst_host)",
                           sim_event_get_sql_fields ());
  return query;
}

gchar *
sim_event_idm_get_insert_clause (GdaConnection *conn, SimEvent *event)
{
  gchar *query;
  const gchar *header;
  gchar *values;

  header = sim_event_idm_get_insert_clause_header ();
  values = sim_event_idm_get_insert_clause_values (conn, event);

  query = g_strdup_printf ("INSERT INTO idm_data %s VALUES %s", header, values);

  g_free (values);

  return query;
}

const gchar *
sim_event_idm_get_insert_clause_header (void)
{
  return "(event_id, username, domain, from_src)";
}

gchar *
sim_event_idm_get_insert_clause_values (GdaConnection *conn, SimEvent *event)
{
  SimIdmInsertAux insert_aux;

  insert_aux.gstr = g_string_new ("");
  insert_aux.event_id = sim_uuid_get_db_string (event->id);
  insert_aux.conn = conn;

  if (event->src_username)
  {
    insert_aux.from_src = TRUE;
    g_hash_table_foreach (event->src_username, sim_event_idm_get_insert_clause_values_one, &insert_aux);
  }

  if (event->dst_username)
  {
    insert_aux.from_src = FALSE;
    g_hash_table_foreach (event->dst_username, sim_event_idm_get_insert_clause_values_one, &insert_aux);
  }

  /* remove last ',' */
  g_string_truncate (insert_aux.gstr, insert_aux.gstr->len - 1);

  return g_string_free (insert_aux.gstr, FALSE);
}

static void
sim_event_idm_get_insert_clause_values_one (gpointer key, gpointer value, gpointer user_data)
{
  gchar *username;
  gchar *domain;
  SimIdmInsertAux *insert_aux;
  gchar *e_username, *e_domain;

  username = key;
  domain = value;
  insert_aux = user_data;

  e_username = sim_str_escape (username, insert_aux->conn, 0);
  e_domain = sim_str_escape (domain, insert_aux->conn, 0);

  g_string_append_printf (insert_aux->gstr, "(%s, '%s', '%s', %d),", insert_aux->event_id, e_username, e_domain, insert_aux->from_src);

  g_free (e_username);
  g_free (e_domain);
}

gchar *
sim_event_extra_get_insert_clause (GdaConnection *conn, SimEvent *event)
{
  gchar *query;
  const gchar *header;
  gchar *values;

  header = sim_event_extra_get_insert_clause_header ();
  values = sim_event_extra_get_insert_clause_values (conn, event);

  query = g_strdup_printf ("INSERT INTO extra_data %s VALUES %s", header, values);

  g_free (values);

  return query;
}

const gchar *
sim_event_extra_get_insert_clause_header (void)
{
  return "(event_id, data_payload, binary_data)";
}

gchar *
sim_event_extra_get_insert_clause_values (GdaConnection *conn, SimEvent *event)
{
  gchar *payload;
  gchar *binary_data;
  gchar *values;

  /* If event->log has something use it as payload instead of event->data */
  if (event->log)
    payload = sim_str_escape (event->log->str, conn, event->log->len);
  else
    payload = sim_str_escape (event->data, conn, 0);

  if (event->binary_data)
    binary_data = sim_str_escape (event->binary_data, conn, 0);
  else
    binary_data = NULL;

  values = g_strdup_printf ("(%s, '%s', %s%s)",
                            sim_uuid_get_db_string (event->id),
                            payload ? payload : "",
                            binary_data ? "0x" : "",
                            binary_data ? binary_data : "NULL");

  g_free (payload);
  g_free (binary_data);

  return values;
}

const gchar *
sim_event_get_acid_event_insert_clause_header (void)
{
  return "(id,device_id,ctx,timestamp,ip_src,ip_dst,src_net, dst_net,ip_proto,layer4_sport,layer4_dport,ossim_priority,ossim_reliability,ossim_asset_src,ossim_asset_dst,ossim_risk_c,ossim_risk_a, ossim_correlation, plugin_id, plugin_sid, tzone, src_hostname,dst_hostname,src_mac,dst_mac,src_host,dst_host)";
}

/*
 *
 *
 *
 *
 */
gchar*
sim_event_get_alarm_insert_clause (SimDatabase *db_ossim,
                                   SimEvent   *event,
                                   gboolean    removable)
{
  gchar    timebuff[TIMEBUF_SIZE];
  gchar   *timestamp = timebuff;
  GString *query;
  GdaConnection *conn;
  gchar   *e_alarm_stats = NULL;
  gchar   *trfree = NULL;

  g_return_val_if_fail (SIM_IS_EVENT (event), NULL);

  conn = sim_database_get_conn (db_ossim);

  if(event->time_str)
  {
    timestamp = trfree = sim_str_escape (event->time_str, conn, 0);
  }
  else
    sim_time_t_to_str (timestamp, event->time);

  guint efr =  event->priority * event->reliability * 2; //this is used for compliance. The "*2" is to take a percentage

  if (event->alarm_stats)
    e_alarm_stats = sim_str_escape (event->alarm_stats, conn, 0);

  ossim_debug ( "%s: risk_c:%f, risk_a:%f", __func__, event->risk_c, event->risk_a);

  query = g_string_new ("REPLACE INTO alarm "
                        "(event_id, backlog_id, corr_engine_ctx, timestamp, plugin_id, plugin_sid, "
                        "protocol, src_ip, dst_ip, src_port, dst_port, "
                        "risk, efr, similar, removable, stats) VALUES (");

  g_string_append_printf (query, "%s", sim_uuid_get_db_string (event->id));
  g_string_append_printf (query, ",%s", sim_uuid_get_db_string (event->backlog_id));
  g_string_append_printf (query, ",%s", sim_uuid_get_db_string (sim_engine_get_id (event->engine)));
  g_string_append_printf (query, ",'%s'", timestamp);
  g_string_append_printf (query, ",%d", event->plugin_id);
  g_string_append_printf (query, ",%d", event->plugin_sid);
  g_string_append_printf (query, ",%d", event->protocol);
  g_string_append_printf (query, ",%s", (event->src_ia) ? sim_inet_get_db_string (event->src_ia) : "NULL");
  g_string_append_printf (query, ",%s", (event->dst_ia) ? sim_inet_get_db_string (event->dst_ia) : "NULL");
  g_string_append_printf (query, ",%d", event->src_port);
  g_string_append_printf (query, ",%d", event->dst_port);
  g_string_append_printf (query, ",%d", ((gint)event->risk_a > (gint)event->risk_c) ? (gint)event->risk_a : (gint)event->risk_c);
  g_string_append_printf (query, ",%u", efr);
  g_string_append_printf (query, ",IF('%s'<>''", (event->groupalarmsha1 != NULL ? event->groupalarmsha1 : ""));
  g_string_append_printf (query, ",'%s'", (event->groupalarmsha1 != NULL ? event->groupalarmsha1 : ""));
  g_string_append_printf (query, ",SHA1('%s'))", sim_uuid_get_db_string (event->id));
  g_string_append_printf (query, ",%d", removable);
  g_string_append_printf (query, ",'%s')", e_alarm_stats ? e_alarm_stats : "");

  g_free (e_alarm_stats);
  g_free (trfree);

  return g_string_free (query, FALSE);
}

const char *
sim_event_pulses_get_insert_clause_header (void)
{
  return "(event_id, pulse_id, ioc_hash, ioc_value)";  
}

static void
_sim_event_otx_data_iter (gpointer key, gpointer value, gpointer userdata)
{
  SimOtxDataIter *pdata = (gpointer)userdata;
  GPtrArray  * array = (GPtrArray*) value;
  guint i;
  for (i = 0; i < array->len; i++)
  {
    gchar *ioc = (gchar *)g_ptr_array_index(array, i);
    gchar *e_ioc = NULL;
    e_ioc = sim_str_escape (ioc,pdata->conn,0);
    g_string_append_printf(pdata->st,"(%s,0x%s,MD5('%s'),'%s'),",
      sim_uuid_get_db_string (pdata->uuid),
      (gchar *)key,
      e_ioc,
      e_ioc);
    g_free (e_ioc);
       
  }
}

gchar *
sim_event_pulses_get_insert_clause_values (GdaConnection  *conn, SimEvent *event)
{
  g_return_val_if_fail (conn != NULL && event != NULL, NULL);
  g_return_val_if_fail (SIM_IS_EVENT (event), NULL);
  gchar *result = NULL;
  SimOtxDataIter data;
  GString * st = NULL;
  data.st = st = g_string_new ("");
  data.uuid = event->id;
  data.conn = conn;
  g_hash_table_foreach (event->otx_data, _sim_event_otx_data_iter, (gpointer)&data);
  g_string_truncate (st, st->len-1); 
  result = g_string_free (st, FALSE);
  return result;
}


gchar * sim_event_pulses_get_insert_clause (GdaConnection *conn, SimEvent *event)
{ 
  gchar *query;
  const gchar *header;
  gchar *values;

  header = sim_event_pulses_get_insert_clause_header();
  values = sim_event_pulses_get_insert_clause_values (conn, event);

  query = g_strdup_printf ("INSERT INTO otx_data %s VALUES %s", header, values);

  values = sim_event_pulses_get_insert_clause_values (conn, event);

  g_free (values);

  return query;
}


/**
 * sim_event_to_string:
 * @event: a #SimEvent object.
 *
 */
gchar *
sim_event_to_string (SimEvent * event)
{
  GString *str;
  gchar *ip;
  gchar * base64;
  gint    base64_len;
  SimUuid * net_id;

  g_return_val_if_fail(SIM_IS_EVENT (event), NULL);

  str = g_string_new("event ");

  g_string_append_printf(str, "event_id=\"%s\" ", sim_uuid_get_string (event->id));
  g_string_append_printf(str, "ctx=\"%s\" ", sim_uuid_get_string (sim_context_get_id (event->context)));
  g_string_append_printf(str, "alarm=\"%d\" ", event->alarm);
  str = g_string_append (str, "is_remote=\"1\" ");

  gchar *aux = sim_event_get_str_from_type(event->type);
  if (aux)
  {
    g_string_append_printf(str, "type=\"%s\" ", aux);
    g_free(aux);
  }

  g_string_append_printf(str, "date=\"%u\" ", (guint)event->time);
  g_string_append_printf(str, "tzone=\"%4.2f\" ", event->tzone);

  if (event->time_str)
    g_string_append_printf(str, "fdate=\"%s\" ", event->time_str);

  if (event->plugin_id)
    g_string_append_printf(str, "plugin_id=\"%d\" ", event->plugin_id);

  if (event->plugin_sid)
    g_string_append_printf(str, "plugin_sid=\"%d\" ", event->plugin_sid);

  if (event->src_ia)
  {
    ip = sim_inet_get_canonical_name (event->src_ia);
    g_string_append_printf (str, "src_ip=\"%s\" ", ip);
    g_free (ip);
  }

  if (event->src_port)
    g_string_append_printf(str, "src_port=\"%d\" ", event->src_port);

  if (event->dst_ia)
  {
    ip = sim_inet_get_canonical_name (event->dst_ia);
    g_string_append_printf (str, "dst_ip=\"%s\" ", ip);
    g_free (ip);
  }

  if (event->dst_port)
    g_string_append_printf(str, "dst_port=\"%d\" ", event->dst_port);

  if (event->src_net)
  {
    net_id = sim_net_get_id (event->src_net);
    g_string_append_printf (str, "src_net=\"%s\" ", sim_uuid_get_string (net_id));
  }

  if (event->dst_net)
  {
    net_id = sim_net_get_id (event->dst_net);
    g_string_append_printf (str, "dst_net=\"%s\" ", sim_uuid_get_string (net_id));
  }

  if (event->sensor)
  {
    ip = sim_inet_get_canonical_name (event->sensor);
    g_string_append_printf(str, "sensor=\"%s\" ", ip);
    g_free (ip);
  }
  if (event->sensor_id)
    g_string_append_printf(str, "sensor_id=\"%s\" ", sim_uuid_get_string (event->sensor_id));

  if (event->device)
  {
    ip = sim_inet_get_canonical_name (event->device);
    g_string_append_printf(str, "device=\"%s\" ", ip);
    g_free (ip);
  }

  if (event->device_id)
    g_string_append_printf (str, "device_id=\"%d\" ", event->device_id);

#if 0
  if (event->server)
    g_string_append_printf (str, "server=\"%s\" ", event->server);
#endif

  if (event->interface)
    g_string_append_printf(str, "interface=\"%s\" ", event->interface);

  if (event->protocol)
  {
    gchar *value = sim_protocol_get_str_from_type(event->protocol);
    g_string_append_printf(str, "protocol=\"%s\" ", value);
    g_free(value);
  }

  if (event->condition)
  {
    gchar *value = sim_condition_get_str_from_type(event->condition);
    g_string_append_printf(str, "condition=\"%s\" ", value);
    g_free(value);
  }
  if (event->value)
    g_string_append_printf(str, "value=\"%s\" ", event->value);
  if (event->interval)
    g_string_append_printf(str, "interval=\"%d\" ", event->interval);

  if (event->is_priority_set)
    g_string_append_printf(str, "priority=\"%d\" ", event->priority);
  if (event->is_reliability_set)
    g_string_append_printf(str, "reliability=\"%d\" ", event->reliability);

  g_string_append_printf(str, "asset_src=\"%d\" ", event->asset_src);
  g_string_append_printf(str, "asset_dst=\"%d\" ", event->asset_dst);

  if (event->risk_c)
    g_string_append_printf(str, "risk_a=\"%lf\" ", event->risk_a);
  if (event->risk_a)
    g_string_append_printf(str, "risk_c=\"%lf\" ", event->risk_c);

  // Only forward this field if this is a special event.
  if ((event->data) && sim_event_is_special (event))
  {
    gchar *base64;
    base64 = g_base64_encode ((guchar *)event->data, strlen(event->data));
    if(base64 == NULL)
      return g_string_free(str, TRUE);
    g_string_append_printf (str, "data=\"%s\" ", base64);
    g_free (base64);
  }

  if (event->log)
  {
    base64 = g_base64_encode((guchar*)event->log->str, event->log->len);
    if(base64 == NULL)
      return g_string_free(str, TRUE);
    g_string_append_printf(str, "log=\"%s\" ", base64);
    g_free(base64);
  }
  if (event->filename && (base64_len = strlen(event->filename)))
  {
    base64 = g_base64_encode( (guchar*)event->filename, base64_len);
    if(base64 == NULL)
      return g_string_free(str, TRUE);
    g_string_append_printf(str, "filename=\"%s\" ", base64);
    g_free(base64);
  }
  if (event->username && (base64_len = strlen(event->username)))
  {
    base64 = g_base64_encode( (guchar*)event->username, base64_len);
    if(base64 == NULL)
      return g_string_free(str, TRUE);
    g_string_append_printf(str, "username=\"%s\" ", base64);
    g_free(base64);
  }
  if (event->password && (base64_len = strlen(event->password)))
  {
    base64 = g_base64_encode( (guchar*) event->password, base64_len);
    if(base64 == NULL)
      return g_string_free(str, TRUE);
    g_string_append_printf(str, "password=\"%s\" ", base64);
    g_free(base64);
  }
  if (event->userdata1 && (base64_len = strlen(event->userdata1)))
  {
    base64 = g_base64_encode( (guchar*)event->userdata1, base64_len);
    if(base64 == NULL)
      return g_string_free(str, TRUE);
    g_string_append_printf(str, "userdata1=\"%s\" ", base64);
    g_free(base64);
  }
  if (event->userdata2 && (base64_len = strlen(event->userdata2)))
  {
    base64 = g_base64_encode( (guchar*)event->userdata2, base64_len);
    if(base64 == NULL)
      return g_string_free(str, TRUE);
    g_string_append_printf(str, "userdata2=\"%s\" ", base64);
    g_free(base64);
  }
  if (event->userdata3 && (base64_len = strlen(event->userdata3)))
  {
    base64 = g_base64_encode( (guchar*)event->userdata3, base64_len);
    if(base64 == NULL)
      return g_string_free(str, TRUE);
    g_string_append_printf(str, "userdata3=\"%s\" ", base64);
    g_free(base64);
  }
  if (event->userdata4 && (base64_len = strlen(event->userdata4)))
  {
    base64 = g_base64_encode( (guchar*)event->userdata4, base64_len);
    if(base64 == NULL)
      return g_string_free(str, TRUE);
    g_string_append_printf(str, "userdata4=\"%s\" ", base64);
    g_free (base64);
  }
  if (event->userdata5 && (base64_len = strlen(event->userdata5)))
  {
    base64 = g_base64_encode( (guchar*)event->userdata5, base64_len);
    if(base64 == NULL)
      return g_string_free(str, TRUE);
    g_string_append_printf(str, "userdata5=\"%s\" ", base64);
    g_free(base64);
  }
  if (event->userdata6 && (base64_len = strlen(event->userdata6)))
  {
    base64 = g_base64_encode( (guchar*)event->userdata6, base64_len);
    if(base64 == NULL)
      return g_string_free(str, TRUE);
    g_string_append_printf(str, "userdata6=\"%s\" ", base64);
    g_free(base64);
  }
  if (event->userdata7 && (base64_len = strlen(event->userdata7)))
  {
    base64 = g_base64_encode( (guchar*)event->userdata7, base64_len);
    if(base64 == NULL)
      return g_string_free(str, TRUE);
    g_string_append_printf(str, "userdata7=\"%s\" ", base64);
    g_free(base64);
  }
  if (event->userdata8 && (base64_len = strlen(event->userdata8)))
  {
    base64 = g_base64_encode( (guchar*)event->userdata8, base64_len);
    if(base64 == NULL)
      return g_string_free(str, TRUE);
    g_string_append_printf(str, "userdata8=\"%s\" ", base64);
    g_free(base64);
  }
  if (event->userdata9 && (base64_len = strlen(event->userdata9)))
  {
    base64 = g_base64_encode( (guchar*)event->userdata9, base64_len);
    if(base64 == NULL)
      return g_string_free(str, TRUE);
    g_string_append_printf(str, "userdata9=\"%s\" ", base64);
    g_free(base64);
  }
  if (event->src_username_raw && (base64_len = strlen(event->src_username_raw)))
  {
    base64 = g_base64_encode ((guchar *)event->src_username_raw, base64_len);
    if(base64 == NULL)
      return g_string_free(str, TRUE);
    g_string_append_printf(str, "src_username=\"%s\" ", base64);
    g_free (base64);
  }
  if (event->dst_username_raw && (base64_len = strlen (event->dst_username_raw)))
  {
    base64 = g_base64_encode ((guchar *)event->dst_username_raw, base64_len);
    if(base64 == NULL)
      return g_string_free(str, TRUE);
    g_string_append_printf(str, "dst_username=\"%s\" ", base64);
    g_free (base64);
  }
  if (event->src_id)
    g_string_append_printf(str, "src_id=\"%s\" ", sim_uuid_get_string (event->src_id));
  if (event->dst_id)
    g_string_append_printf(str, "dst_id=\"%s\" ", sim_uuid_get_string (event->dst_id));
  if (event->src_hostname)
    g_string_append_printf(str, "src_hostname=\"%s\" ", event->src_hostname);
  if (event->dst_hostname)
    g_string_append_printf(str, "dst_hostname=\"%s\" ", event->dst_hostname);
  if (event->src_mac)
    g_string_append_printf(str, "src_mac=\"%s\" ", event->src_mac);
  if (event->dst_mac)
    g_string_append_printf(str, "dst_mac=\"%s\" ", event->dst_mac);
  if (event->rep_prio_src)
    g_string_append_printf(str, "rep_prio_src=\"%u\" ", event->rep_prio_src);

  if (event->rep_prio_dst)
    g_string_append_printf(str, "rep_prio_dst=\"%u\" ", event->rep_prio_dst);

  if (event->rep_rel_src)
    g_string_append_printf(str, "rep_rel_src=\"%u\" ", event->rep_rel_src);

  if (event->rep_rel_dst)
    g_string_append_printf(str, "rep_rel_dst=\"%u\" ", event->rep_rel_dst);

  if (event->str_rep_act_src && (base64_len = strlen(event->str_rep_act_src)))
  {
    base64 = g_base64_encode( (guchar*)event->str_rep_act_src, base64_len);
    if(base64 == NULL)
      return g_string_free(str, TRUE);
    g_string_append_printf(str, "rep_act_src=\"%s\" ", base64);
    g_free(base64);
  }

  if (event->str_rep_act_dst && (base64_len = strlen(event->str_rep_act_dst)))
  {
    base64 = g_base64_encode( (guchar*)event->str_rep_act_dst, base64_len);
    if(base64 == NULL)
      return g_string_free(str, TRUE);
    g_string_append_printf(str, "rep_act_dst=\"%s\" ", base64);
    g_free(base64);
  }

  /* We need to check that the */
  if (event->binary_data != NULL)
  {
    g_string_append_printf(str,"binary_data=\"%s\" ", event->binary_data);
  }

  g_string_append_printf(str, "\n");

  return g_string_free(str, FALSE);
}

/*
 * Returns TRUE if the event is one of the "special" events: MAC, OS, Service or HIDS
 */
gboolean
sim_event_is_special (SimEvent *event)
{
  if ((event->plugin_id == 1512) ||
      (event->plugin_id == 1511) ||
      (event->plugin_id == 1516) ||
      (event->plugin_id == 4001))
    return TRUE;
  else
    return FALSE;
}

gboolean
sim_event_set_sid (SimEvent *event)
{
  gchar *device_ip;
  gchar *sensor_device;
  guint sid;

  g_return_val_if_fail (SIM_IS_EVENT (event), FALSE);

  ossim_debug ("%s: Setting (sid)  for event->id=%s", __func__, sim_uuid_get_string (event->id));

  if (event->device)
  {
    device_ip = sim_inet_get_canonical_name (event->device);
    sensor_device = g_strdup_printf ("%s/%s/%s", sim_uuid_get_string (event->sensor_id), event->interface, device_ip);
    g_free (device_ip);
  }
  else
    sensor_device = g_strdup_printf ("%s", sim_uuid_get_string (event->sensor_id));

  sid = sim_container_get_sensor_sid (ossim.container, sensor_device);

  if (!sid)                     // First event for this sid, so we must insert the sensor to the database
  {
    // Insert it to db as the old way (but now it's cached).
    sid = sim_organizer_snort_sensor_get_sid (ossim.dbsnort, event->sensor_id, event);
    sim_container_add_sensor_sid (ossim.container, sensor_device, sid);
    ossim_debug ("%s: not from cache: sid: %u", __func__, sid);
  }
  else
  {
    ossim_debug ("%s: from cache: sid: %u", __func__, sid);
    g_free (sensor_device);
  }

  event->device_id = sid;

  return TRUE;
}

void
sim_event_enrich_idm (SimEvent *event)
{
	SimIdmEntry *entry;
	// IDM queries only if the IDM info is empty, usefull for not overwriting forwarded events

	if (!event->src_username && !event->src_hostname && !event->src_mac && !event->src_id)
	{
		entry = sim_idm_get (sim_context_get_id (event->context), event->src_ia);

		if (entry)
		{
			const gchar *value;

			value = sim_idm_entry_get_username (entry);
	    if (value)
		  {
				event->src_username_raw = g_strdup (value);
			  event->src_username = sim_command_idm_event_parse_username (value);
       }

			value = sim_idm_entry_get_hostname (entry);
      if (value)
	      event->src_hostname = g_strdup(value);

			value = sim_idm_entry_get_mac(entry);
		  if (value)
			  event->src_mac = g_strdup(value);

			event->src_id = g_object_ref (sim_idm_entry_get_host_id (entry));

			g_object_unref (entry);
		}
	}
	if (!event->dst_username && !event->dst_hostname && !event->dst_mac && !event->dst_id)
	{
		entry = sim_idm_get (sim_context_get_id (event->context), event->dst_ia);

		if (entry)
		{
			const gchar *value;

			value = sim_idm_entry_get_username (entry);
	    if (value)
		  {
				event->dst_username_raw = g_strdup (value);
			  event->dst_username = sim_command_idm_event_parse_username (value);
       }

			value = sim_idm_entry_get_hostname (entry);
      if (value)
	      event->dst_hostname = g_strdup(value);

			value = sim_idm_entry_get_mac(entry);
		  if (value)
			  event->dst_mac = g_strdup(value);

			event->dst_id = g_object_ref (sim_idm_entry_get_host_id (entry));

			g_object_unref (entry);
		}
	}
}

void
sim_event_set_asset_value (SimEvent *event)
{
  if (event->asset_src == VOID_ASSET)
  {
    event->asset_src = DEFAULT_ASSET;
    if (event->src_id)
      event->asset_src = sim_context_get_host_asset (event->context, event->src_id);
    else
      if (event->src_ia)
        event->asset_src = sim_context_get_inet_asset (event->context, event->src_ia);
  }

  if (event->asset_dst == VOID_ASSET)
  {
    event->asset_dst = DEFAULT_ASSET;
    if (event->dst_id)
      event->asset_dst = sim_context_get_host_asset (event->context, event->dst_id);
    else
      if (event->dst_ia)
        event->asset_dst = sim_context_get_inet_asset (event->context, event->dst_ia);
  }
}

/*
 * This function checks if some policy matches with the event, and sssociates the event to the policy. It also sets 
 * the event role (first it takes the server role, if any, after that, the specific policy event role)
 */
gboolean
sim_event_set_role_and_policy (SimEvent *event)
{
  g_return_val_if_fail (SIM_IS_EVENT (event), FALSE);

  //now we can segregate and tell this server to do a specific  thing.
  //For example we can decide that this server will be able to qualify events, but not to correlate them.
  event->policy = sim_context_get_event_policy (event->context, event);

  //The policy role (if any) supersedes the general server role.
  if (event->policy && sim_policy_has_role (event->policy))
  {
    event->role = sim_policy_get_role (event->policy);
  }
  else // Uses general server role
  {
    SimConfig *config = sim_server_get_config (ossim.server);
    event->role = sim_config_get_server_role (config);
  }

  return TRUE;
}

const gchar *sim_event_get_sql_fields (void)
{
	return sql_text_fields;
}

/**
 * sim_event_set_src_host_properties:
 *
 */
void
sim_event_set_src_host_properties (SimEvent *event,
                                   SimHost  *host)
{
  SimUuid *host_id;

  g_return_if_fail (SIM_IS_EVENT (event));
  g_return_if_fail (SIM_IS_HOST (host));

  host_id = sim_host_get_id (host);

  if (event->src_id)
    g_object_unref (event->src_id);

  event->src_id = g_object_ref (host_id);

  event->asset_src = sim_host_get_asset (host);
}

/**
 * sim_event_set_dst_host_properties:
 *
 */
void
sim_event_set_dst_host_properties (SimEvent *event,
                                   SimHost  *host)
{
  SimUuid *host_id;

  g_return_if_fail (SIM_IS_EVENT (event));
  g_return_if_fail (SIM_IS_HOST (host));

  host_id = sim_host_get_id (host);

  if (event->dst_id)
    g_object_unref (event->dst_id);

  event->dst_id = g_object_ref (host_id);

  event->asset_dst = sim_host_get_asset (host);
}

/**
 * sim_event_set_context_and_engine:
 * @event: a #SimEvent
 * @context_id: a #SimUuid
 *
 */
void
sim_event_set_context_and_engine (SimEvent *event,
                                  SimUuid  *context_id)
{
  SimContext *context;
  SimEngine  *engine;

  g_return_if_fail (SIM_IS_EVENT (event));

  if (event->context)
    g_object_unref (event->context);
  if (event->engine)
    g_object_unref (event->engine);


  context = sim_container_get_context (ossim.container, context_id);
  if (!context)
  {
    g_message ("%s: Error getting context_id %s", __func__, context_id ? sim_uuid_get_string (context_id) : "NULL");
    return;
  }
  engine = sim_container_get_engine_for_context (ossim.container, context_id);
  if (!engine)
  {
    g_message ("%s: Error getting engine for context_id %s", __func__, context_id ? sim_uuid_get_string (context_id) : "NULL");
    return;
  }

  event->context = g_object_ref (context);
  event->engine = g_object_ref (engine);
}

void
sim_event_add_ioc (SimEvent * event, const gchar *pulse_id, const gchar *ioc)
{
  GPtrArray *array; 
  g_return_if_fail (event != NULL && pulse_id != NULL && ioc != NULL);
  if ((array = g_hash_table_lookup (event->otx_data, pulse_id)) != NULL)
  {
    g_ptr_array_add (array, g_strdup (ioc));
  }
  else
  {
    array = g_ptr_array_new_with_free_func(g_free);
    g_ptr_array_add (array, g_strdup (ioc));
    g_hash_table_insert (event->otx_data, g_strdup (pulse_id), array);
  }
}

bson_t *
sim_event_to_bson (SimEvent * event)
{
  g_return_val_if_fail (event != NULL, NULL);
  g_return_val_if_fail (SIM_IS_EVENT (event), NULL);
  gchar *json = NULL;
  bson_t *bson = NULL;
  bson_error_t berror;
  if ((json = sim_event_to_json (event)) != NULL)
  {
    bson = bson_new_from_json ((const uint8_t *)json, strlen(json), &berror);
    if (!bson)
    {
      g_message ("Can't convert Event to BSON");
      ossim_debug("Event JSON object:%s", json);  
    }
  }
  if (json)
    g_free (json);
  return bson;
}

gchar *
sim_event_to_json (SimEvent *event)
{
  g_return_val_if_fail (event != NULL, NULL);
  g_return_val_if_fail (SIM_IS_EVENT(event), NULL);

  gchar *str;
  gchar *event_uuid;
  gchar *event_type;
  gchar *event_ctx;
  JsonBuilder *builder;
  JsonGenerator *generator;
  JsonNode *root;

  if ((event_uuid = sim_uuid_to_base64 (event->id)) == NULL)
  {
    return NULL;
  }
  if ((event_type = sim_event_get_str_from_type (event->type)) == NULL)
  {
    g_free (event_uuid);
    return NULL;
  }
  if ((event_ctx = sim_uuid_to_base64 (sim_context_get_id (event->context))) == NULL)
  {
    g_free (event_uuid);
    g_free (event_type);
    return NULL;
  }

  builder = json_builder_new ();

  json_builder_begin_object (builder);
  json_builder_set_member_name (builder, "event");
  json_builder_begin_object (builder);

  /* event_id */
  json_builder_set_member_name (builder, "event_id");
  json_builder_begin_object (builder);
  json_builder_set_member_name (builder, "$binary");
  json_builder_add_string_value (builder, event_uuid);
  json_builder_set_member_name (builder, "$type");
  json_builder_add_string_value (builder, "04");
  json_builder_end_object (builder);
  g_free (event_uuid);

  /* type */
  json_builder_set_member_name (builder, "type");
  json_builder_add_string_value (builder, event_type);
  g_free (event_type);

  /* ctx*/
  json_builder_set_member_name (builder, "ctx");
  json_builder_begin_object (builder);
  json_builder_set_member_name (builder, "$binary");
  json_builder_add_string_value (builder, event_ctx);
  json_builder_set_member_name (builder, "$type");
  json_builder_add_string_value (builder, "04");
  json_builder_end_object (builder);
  g_free (event_ctx);

  /* alarm */
  json_builder_set_member_name (builder, "alarm");
  json_builder_add_boolean_value (builder, event->alarm);

  /* is_remote */
  if (sim_role_sim (ossim.config->server.role))
  {
    json_builder_set_member_name (builder, "is_remote");
    json_builder_add_boolean_value (builder, event->is_remote);
  }

  /* date */
  str = g_strdup_printf("%lu", event->time);
  json_builder_set_member_name (builder, "date");
  json_builder_begin_object (builder);
  json_builder_set_member_name (builder, "$numberLong");
  json_builder_add_string_value (builder, str);
  json_builder_end_object (builder);
  g_free (str);

  /* tzone */
  /* !! HACK !!
    Json-glib dumps gdouble values without decimal part as integers
    when the decimal part is 0, for instance 4.0 becomes 4.
    This makes the bson_new_from_json to generate events with wrong format.
    To prevent this, a minimum offset is added to gdoubles values to generate
    valid json and bson messages. The offset is ignored on the message reception.
  */
  json_builder_set_member_name (builder, "tzone");
  if (event->tzone > 0)
    json_builder_add_double_value (builder, event->tzone + 0.00000000001);
  else
    json_builder_add_double_value (builder, event->tzone - 0.00000000001);

  /* fdate */
  if (event->time_str)
  {
    json_builder_set_member_name (builder, "fdate");
    json_builder_add_string_value (builder, event->time_str);
  }

  /* plugin_id */
  if (event->plugin_id)
  {
    json_builder_set_member_name (builder, "plugin_id");
    json_builder_add_int_value (builder, event->plugin_id);
  }

  /* plugin_sid */
  if (event->plugin_sid)
  {
    json_builder_set_member_name (builder, "plugin_sid");
    json_builder_add_int_value (builder, event->plugin_sid);
  }

  /* src_ip */
  if (event->src_ia)
  {
    str = sim_inet_get_canonical_name (event->src_ia);
    json_builder_set_member_name (builder, "src_ip");
    json_builder_add_string_value (builder, str);
    g_free (str);
  }

  /* dst_ip */
  if (event->dst_ia)
  {
    str = sim_inet_get_canonical_name (event->dst_ia);
    json_builder_set_member_name (builder, "dst_ip");
    json_builder_add_string_value (builder, str);
    g_free (str);
  }

  /* src_port */
  if (event->src_port)
  {
    json_builder_set_member_name (builder, "src_port");
    json_builder_add_int_value (builder, event->src_port);
  }

  /* dst_port */
  if (event->dst_port)
  {
    json_builder_set_member_name (builder, "dst_port");
    json_builder_add_int_value (builder, event->dst_port);
  }

  /* src_net */
  if (event->src_net)
  {
    SimUuid *net_id = sim_net_get_id (event->src_net);
    if ((str = sim_uuid_to_base64 (net_id)) != NULL)
    {
      json_builder_set_member_name (builder, "src_net");
      json_builder_begin_object (builder);
      json_builder_set_member_name (builder, "$binary");
      json_builder_add_string_value (builder, str);
      json_builder_set_member_name (builder, "$type");
      json_builder_add_string_value (builder, "04");
      json_builder_end_object (builder);
      g_free (str);
    }
  }

  /* dst_net */
  if (event->dst_net)
  {
    SimUuid *net_id = sim_net_get_id (event->dst_net);
    if ((str = sim_uuid_to_base64 (net_id)) != NULL)
    {
      json_builder_set_member_name (builder, "dst_net");
      json_builder_begin_object (builder);
      json_builder_set_member_name (builder, "$binary");
      json_builder_add_string_value (builder, str);
      json_builder_set_member_name (builder, "$type");
      json_builder_add_string_value (builder, "04");
      json_builder_end_object (builder);
      g_free (str);
    }
  }

  /* sensor */
  if (event->sensor)
  {
    str = sim_inet_get_canonical_name (event->sensor);
    json_builder_set_member_name (builder, "sensor");
    json_builder_add_string_value (builder, str);
    g_free (str);
  }

  /* sensor_id */
  if (event->sensor_id)
  {
    if ((str = sim_uuid_to_base64 (event->sensor_id)) != NULL)
    {
      json_builder_set_member_name (builder, "sensor_id");
      json_builder_begin_object (builder);
      json_builder_set_member_name (builder, "$binary");
      json_builder_add_string_value (builder, str);
      json_builder_set_member_name (builder, "$type");
      json_builder_add_string_value (builder, "04");
      json_builder_end_object (builder);
      g_free (str);
    }
  }

  /* device */
  if (event->device)
  {
    str = sim_inet_get_canonical_name (event->device);
    json_builder_set_member_name (builder, "device");
    json_builder_add_string_value (builder, str);
    g_free (str);
  }

  /* interface */
  if (event->interface)
  {
    json_builder_set_member_name (builder, "interface");
    json_builder_add_string_value (builder, event->interface);
  }

  /* protocol */
  if (event->protocol)
  {
    str = sim_protocol_get_str_from_type (event->protocol);
    json_builder_set_member_name (builder, "protocol");
    json_builder_add_string_value (builder, str);
  }

  /* condition */
  if (event->condition)
  {
    str = sim_condition_get_str_from_type (event->condition);
    json_builder_set_member_name (builder, "condition");
    json_builder_add_string_value (builder, str);
  }

  /* value */
  if (event->value)
  {
    json_builder_set_member_name (builder, "value");
    json_builder_add_string_value (builder, event->value);
  }

  /* interval */
  if (event->interval)
  {
    json_builder_set_member_name (builder, "interval");
    json_builder_add_int_value (builder, event->interval);
  }

  /* priority */
  if (event->is_priority_set)
  {
    json_builder_set_member_name (builder, "priority");
    json_builder_add_int_value (builder, event->priority);
  }

  /* reliability */
  if (event->is_reliability_set)
  {
    json_builder_set_member_name (builder, "reliability");
    json_builder_add_int_value (builder, event->reliability);
  }

  /* asset_src */
  json_builder_set_member_name (builder, "asset_src");
  json_builder_add_int_value (builder, event->asset_src);

  /* asset_dst */
  json_builder_set_member_name (builder, "asset_dst");
  json_builder_add_int_value (builder, event->asset_dst);

  /* risk_a */
  if (event->risk_a != DEFAULT_RISK)
  {
    json_builder_set_member_name (builder, "risk_a");
    /* !! HACK !!
      Json-glib dumps gdouble values without decimal part as integers
      when the decimal part is 0, for instance 4.0 becomes 4.
      This makes the bson_new_from_json to generate events with wrong format.
      To prevent this, a minimum offset is added to gdoubles values to generate
      valid json and bson messages. The offset is ignored on the message reception.
    */
    json_builder_add_double_value (builder, event->risk_a + 0.00000000001);
  }

  /* risk_c */
  if (event->risk_c != DEFAULT_RISK)
  {
    json_builder_set_member_name (builder, "risk_c");
    /* !! HACK !!
      Json-glib dumps gdouble values without decimal part as integers
      when the decimal part is 0, for instance 4.0 becomes 4.
      This makes the bson_new_from_json to generate events with wrong format.
      To prevent this, a minimum offset is added to gdoubles values to generate
      valid json and bson messages. The offset is ignored on the message reception.
    */
    json_builder_add_double_value (builder, event->risk_c + 0.00000000001);
  }

  /* log */
  if (event->log)
  {
    str = g_base64_encode((guchar*)event->log->str, event->log->len);
    if (str != NULL)
    {
      json_builder_set_member_name (builder, "log");
      json_builder_begin_object (builder);
      json_builder_set_member_name (builder, "$binary");
      json_builder_add_string_value (builder, str);
      json_builder_set_member_name (builder, "$type");
      json_builder_add_string_value (builder, "00");
      json_builder_end_object (builder);
      g_free (str);
    }
  }

  /* filename */
  if (event->filename)
  {
    json_builder_set_member_name (builder, "filename");
    json_builder_add_string_value (builder, event->filename);
  }

  /* username */
  if (event->username)
  {
    json_builder_set_member_name (builder, "username");
    json_builder_add_string_value (builder, event->username);
  }

  /* password */
  if (event->password)
  {
    json_builder_set_member_name (builder, "password");
    json_builder_add_string_value (builder, event->password);
  }

  /* userdata1 */
  if (event->userdata1)
  {
    json_builder_set_member_name (builder, "userdata1");
    json_builder_add_string_value (builder, event->userdata1);
  }

  /* userdata2 */
  if (event->userdata2)
  {
    json_builder_set_member_name (builder, "userdata2");
    json_builder_add_string_value (builder, event->userdata2);
  }

  /* userdata3 */
  if (event->userdata3)
  {
    json_builder_set_member_name (builder, "userdata3");
    json_builder_add_string_value (builder, event->userdata3);
  }

  /* userdata4 */
  if (event->userdata4)
  {
    json_builder_set_member_name (builder, "userdata4");
    json_builder_add_string_value (builder, event->userdata4);
  }

  /* userdata5 */
  if (event->userdata5)
  {
    json_builder_set_member_name (builder, "userdata5");
    json_builder_add_string_value (builder, event->userdata5);
  }

  /* userdata6 */
  if (event->userdata6)
  {
    json_builder_set_member_name (builder, "userdata6");
    json_builder_add_string_value (builder, event->userdata6);
  }

  /* userdata7 */
  if (event->userdata7)
  {
    json_builder_set_member_name (builder, "userdata7");
    json_builder_add_string_value (builder, event->userdata7);
  }

  /* userdata8 */
  if (event->userdata8)
  {
    json_builder_set_member_name (builder, "userdata8");
    json_builder_add_string_value (builder, event->userdata8);
  }

  /* userdata9 */
  if (event->userdata9)
  {
    json_builder_set_member_name (builder, "userdata9");
    json_builder_add_string_value (builder, event->userdata9);
  }

  /* src_username */
  if (event->src_username_raw)
  {
    json_builder_set_member_name (builder, "src_username");
    json_builder_add_string_value (builder, event->src_username_raw);
  }

  /* dst_username */
  if (event->dst_username_raw)
  {
    json_builder_set_member_name (builder, "dst_username");
    json_builder_add_string_value (builder, event->dst_username_raw);
  }

  /* src_id */
  if (event->src_id)
  {
    if ((str = sim_uuid_to_base64 (event->src_id)) != NULL)
    {
      json_builder_set_member_name (builder, "src_id");
      json_builder_begin_object (builder);
      json_builder_set_member_name (builder, "$binary");
      json_builder_add_string_value (builder, str);
      json_builder_set_member_name (builder, "$type");
      json_builder_add_string_value (builder, "04");
      json_builder_end_object (builder);
      g_free (str);
    }
  }

  /* dst_id */
  if (event->dst_id)
  {
    if ((str = sim_uuid_to_base64 (event->dst_id)) != NULL)
    {
      json_builder_set_member_name (builder, "dst_id");
      json_builder_begin_object (builder);
      json_builder_set_member_name (builder, "$binary");
      json_builder_add_string_value (builder, str);
      json_builder_set_member_name (builder, "$type");
      json_builder_add_string_value (builder, "04");
      json_builder_end_object (builder);
      g_free (str);
    }
  }

  /* src_hostname */
  if (event->src_hostname)
  {
    json_builder_set_member_name (builder, "src_hostname");
    json_builder_add_string_value (builder, event->src_hostname);
  }

  /* dst_hostname */
  if (event->dst_hostname)
  {
    json_builder_set_member_name (builder, "dst_hostname");
    json_builder_add_string_value (builder, event->dst_hostname);
  }

  /* src_mac */
  if (event->src_mac)
  {
    json_builder_set_member_name (builder, "src_mac");
    json_builder_add_string_value (builder, event->src_mac);
  }

  /* dst_mac */
  if (event->dst_mac)
  {
    json_builder_set_member_name (builder, "dst_mac");
    json_builder_add_string_value (builder, event->dst_mac);
  }

  /* rep_prio_src */
  if (event->rep_prio_src)
  {
    json_builder_set_member_name (builder, "rep_prio_src");
    json_builder_add_int_value (builder, event->rep_prio_src);
  }

  /* rep_prio_dst */
  if (event->rep_prio_dst)
  {
    json_builder_set_member_name (builder, "rep_prio_dst");
    json_builder_add_int_value (builder, event->rep_prio_dst);
  }

  /* rep_rel_src */
  if (event->rep_rel_src)
  {
    json_builder_set_member_name (builder, "rep_rel_src");
    json_builder_add_int_value (builder, event->rep_rel_src);
  }

  /* rep_rel_dst */
  if (event->rep_rel_dst)
  {
    json_builder_set_member_name (builder, "rep_rel_dst");
    json_builder_add_int_value (builder, event->rep_rel_dst);
  }

  /* rep_act_src */
  if (event->str_rep_act_src)
  {
    json_builder_set_member_name (builder, "rep_act_src");
    json_builder_add_string_value (builder, event->str_rep_act_src);
  }

  /* rep_act_dst */
  if (event->str_rep_act_dst)
  {
    json_builder_set_member_name (builder, "rep_act_dst");
    json_builder_add_string_value (builder, event->str_rep_act_dst);
  }


  /* binary_data */
  if (event->binary_data)
  {
    json_builder_set_member_name (builder, "binary_data");
    json_builder_add_string_value (builder, (gchar *)event->binary_data);
  }

  /* pulses */
  if ((g_hash_table_size (event->otx_data)) > 0)
  {
    GHashTableIter iter;
    const gchar *key;
    GPtrArray  *iocs;
    guint i;

    json_builder_set_member_name (builder, "pulses");
    json_builder_begin_object (builder);

    g_hash_table_iter_init (&iter, event->otx_data);
    while (g_hash_table_iter_next (&iter, (gpointer *)&key, (gpointer *)&iocs))
    {
      json_builder_set_member_name (builder, key);
      json_builder_begin_array (builder);
      for (i = 0 ; i < iocs->len; i++)
      {
        json_builder_add_string_value (builder, (gchar *)g_ptr_array_index (iocs, i));
      }
      json_builder_end_array (builder);
    }
    json_builder_end_object (builder);
  }

  // End "event" object
  json_builder_end_object (builder);
  // End global json object
  json_builder_end_object (builder);

  generator = json_generator_new ();
  root = json_builder_get_root (builder);
  json_generator_set_root (generator, root);
  str = json_generator_to_data (generator, NULL);

  // Free json objects
  json_node_free (root);
  g_object_unref (generator);
  g_object_unref (builder);

  return str;
}

static void 
sim_event_json_uuid (GString *st,const char *key,  JsonNode *node)
{
  g_return_if_fail (st != NULL);
  g_return_if_fail (node != NULL);
  if (JSON_NODE_HOLDS_OBJECT (node))
  {
    /* Read the JSON "binary" */
    JsonReader *uuidreader = json_reader_new (node);
    if (json_reader_read_member (uuidreader, "$binary"))
    {
      /* This is a base64 string. Pass to uuid */
      guchar  * temp;
      gsize size;
      if ( (temp = g_base64_decode (json_reader_get_string_value (uuidreader), &size)) != NULL)
      {
        if (size == 16)
        {
          SimUuid * uuid = sim_uuid_new_from_bin (temp);
          g_string_append_printf (st, " %s=\"%s\"", key, sim_uuid_get_string (uuid));
          g_object_unref (uuid);
        }
        g_free (temp);   
      }
    }
    g_object_unref (uuidreader);
  }
}
static void
sim_event_json_base64 (GString *st, const gchar *key, JsonNode *node)
{
 g_return_if_fail (st != NULL);
  g_return_if_fail (node != NULL);
  if (JSON_NODE_HOLDS_OBJECT (node))
  {
    /* Read the JSON "binary" */
    JsonReader *reader = json_reader_new (node);
    if (json_reader_read_member (reader, "$binary"))
    {
      g_string_append_printf (st," %s=\"%s\"", key, json_reader_get_string_value (reader));
    }
    g_object_unref (reader);
  }
}
/**
  Pass a JSON event format to a text event 
*/
gchar *
sim_event_json_to_string (const char *json)
{
  g_return_val_if_fail (json !=NULL, NULL);
  JsonParser *jp;
  GString * st = g_string_new("event ");
  gchar *result = NULL;
  jp = json_parser_new ();
  if (json_parser_load_from_data (jp, json, strlen (json), NULL))
  {
    JsonNode *root = json_parser_get_root (jp);
    JsonReader *jr = json_reader_new (root);
    if (json_reader_read_member (jr, "event"))
    {
      /* Read the elements */
      JsonNode * value = json_reader_get_value (jr);
      if (JSON_NODE_HOLDS_OBJECT (value))
      {
        /* Parse the values */
        JsonReader *rev = json_reader_new (value);
        /* Now check each value */
        /* The uuid is a bit tricky */
        /* event_id */
        if (json_reader_read_member (rev, "event_id"))
        {
          sim_event_json_uuid (st,"event_id", json_reader_get_value (rev));
          json_reader_end_element (rev);
        }
        /* ctx */
        if (json_reader_read_member (rev, "ctx"))
        {
          sim_event_json_uuid (st, "ctx", json_reader_get_value (rev));
          json_reader_end_element (rev);
        }
        if (json_reader_read_member (rev, "alarm"))
        {
          g_string_append_printf (st, " alarm=\"%d\"", json_reader_get_boolean_value(rev) ? 1 : 0);
          json_reader_end_element (rev);
        }
        /* is_remote */
        if (json_reader_read_member (rev, "is_remote"))
        {
          g_string_append_printf (st, " is_remote=\"1\"");
          json_reader_end_element (rev);
        }
        /* date */
        if (json_reader_read_member (rev, "date"))
        {
          g_string_append_printf (st, " date=\"%lu\"", json_reader_get_int_value (rev));
          json_reader_end_element (rev);
        }
        /* tzone */
        if (json_reader_read_member (rev, "tzone"))
        {
          g_string_append_printf (st, " tzone=\"%4.2f\"", json_reader_get_double_value (rev));
          json_reader_end_element (rev);
        }
        /* fdate */
        if (json_reader_read_member (rev, "fdate"))
        {
          g_string_append_printf (st, "fdate=\"%s\"", json_reader_get_string_value (rev));
          json_reader_end_element (rev);
        }
        /* plugin_id */
        if (json_reader_read_member (rev, "plugin_id"))
        {
          g_string_append_printf (st, " plugin_id=\"%ld\"", json_reader_get_int_value (rev));
          json_reader_end_element (rev);
        }
        /* plugin_sid */
        if (json_reader_read_member (rev, "plugin_sid"))
        {
          g_string_append_printf (st, " plugin_id=\"%ld\"", json_reader_get_int_value (rev));
          json_reader_end_element (rev);
        }
        /* src_ip */
        if (json_reader_read_member (rev, "src_ip"))
        {
          g_string_append_printf (st, " src_ip=\"%s\"", json_reader_get_string_value (rev));
          json_reader_end_element (rev);
        }
        /* src_port */
        if (json_reader_read_member (rev, "src_port"))
        {
          g_string_append_printf (st, " src_port=\"%lu\"", json_reader_get_int_value (rev));
          json_reader_end_element (rev);
        }
        /* dst_ip */
        if (json_reader_read_member (rev, "dst_ip"))
        {
          g_string_append_printf (st, " dst_ip=\"%s\"", json_reader_get_string_value (rev));
          json_reader_end_element (rev);
        }
        /* dst_port */
        if (json_reader_read_member (rev, "dst_port"))
        {
          g_string_append_printf (st, " dst_port=\"%ld\"", json_reader_get_int_value (rev));
          json_reader_end_element (rev);
        }
        /* src_net */
        if (json_reader_read_member (rev, "src_net"))
        {
          sim_event_json_uuid (st,"src_net", json_reader_get_value (rev));
          json_reader_end_element (rev);
        }
        /* dst_net */
        if (json_reader_read_member (rev, "dst_net"))
        {
          sim_event_json_uuid (st,"dst_net", json_reader_get_value (rev));
          json_reader_end_element (rev);
        }
        /* sensor */
        if (json_reader_read_member (rev, "sensor"))
        {
          g_string_append_printf (st, " sensor=\"%s\"", json_reader_get_string_value (rev));
          json_reader_end_element (rev);
        }
        /* sensor_id */
        if (json_reader_read_member (rev, "sensor_id"))
        {
          sim_event_json_uuid (st,"sensor_id", json_reader_get_value (rev));
          json_reader_end_element (rev);
        }
        /* device */
        if (json_reader_read_member (rev, "device"))
        {
          g_string_append_printf (st, " device=\"%s\"", json_reader_get_string_value (rev));
          json_reader_end_element (rev);
        }
        /* interface  */
        if (json_reader_read_member (rev, "interface"))
        {
          g_string_append_printf (st, " interface=\"%s\"", json_reader_get_string_value (rev));
          json_reader_end_element (rev);
        }
        /* Protocol */
        if (json_reader_read_member (rev, "protocol"))
        {
          g_string_append_printf (st, " protocol=\"%s\"", json_reader_get_string_value (rev));
          json_reader_end_element (rev);
        }
        /* Condition */
        if (json_reader_read_member (rev, "condition"))
        {
          g_string_append_printf (st, " condiction=\"%s\"", json_reader_get_string_value (rev));
          json_reader_end_element (rev);
        }
        /* value */
        if (json_reader_read_member (rev, "value"))
        {
          g_string_append_printf (st, " value=\"%s\"", json_reader_get_string_value (rev));
          json_reader_end_element (rev);
        }
        /* interval */
        if (json_reader_read_member (rev, "interval"))
        {
          g_string_append_printf (st, " interval=\"%ld\"", json_reader_get_int_value (rev));
          json_reader_end_element (rev);
        }
        /* priority */
        if (json_reader_read_member (rev, "priority"))
        {
          g_string_append_printf (st, " priority=\"%ld\"", json_reader_get_int_value (rev));
          json_reader_end_element (rev);
        }
        /* reliability */
        if (json_reader_read_member (rev, "reliability"))
        {
          g_string_append_printf (st, " reliability=\"%ld\"", json_reader_get_int_value (rev));
          json_reader_end_element (rev);
        }
        /* asset_src */
        if (json_reader_read_member (rev, "asset_src"))
        {
          g_string_append_printf (st, " asset_src=\"%s\"", json_reader_get_string_value (rev));
          json_reader_end_element (rev);
        }
        /* asset_dst */
        if (json_reader_read_member (rev, "asset_dst"))
        {
          g_string_append_printf (st, " asset_dst=\"%s\"", json_reader_get_string_value (rev));
          json_reader_end_element (rev);
        }
        /* risk a */
        if (json_reader_read_member (rev, "risk_a"))
        {
          g_string_append_printf (st, " risk_a=\"%lf\"", json_reader_get_double_value (rev));
          json_reader_end_element (rev);
        }
        /* risk_c */
        if (json_reader_read_member (rev, "risk_c"))
        {
          g_string_append_printf (st, " risk_c=\"%lf\"", json_reader_get_double_value (rev));
          json_reader_end_element (rev);
        }
        /* event data. Must be base64 encoded */
        gchar *base64keys[]={
          "data",
          "log",
          "filename",
          "username",
          "password",
          "userdata1",
          "userdata2",
          "userdata3",
          "userdata4",
          "userdata5",
          "userdata6",
          "userdata7",
          "userdata8",
          "userdata9",
          "rep_act_src",
          "rep_act_dst"
        };
        guint i;
        for (i = 0; i < G_N_ELEMENTS (base64keys); i++)
        {
          if (json_reader_read_member (rev, base64keys[i]))
          {
            sim_event_json_base64 (st, base64keys[i], json_reader_get_value (rev));
            json_reader_end_element (rev);
          }
        }
        /* src_username */
        if (json_reader_read_member (rev, "src_username"))
        {
          g_string_append_printf (st, " src_username=\"%s\"", json_reader_get_string_value (rev));
          json_reader_end_element (rev);
        }
        /* dst_username */
        if (json_reader_read_member (rev, "dst_username"))
        {
          g_string_append_printf (st, " dst_username=\"%s\"", json_reader_get_string_value (rev));
          json_reader_end_element (rev);
        }
        /* src_id */
        if (json_reader_read_member (rev, "src_id"))
        {
          sim_event_json_uuid (st, "src_id", json_reader_get_value(rev));
          json_reader_end_element (rev);
        }
        /* dst_id */
        if (json_reader_read_member (rev, "dst_id"))
        {
          sim_event_json_uuid (st, "dst_id", json_reader_get_value(rev));
          json_reader_end_element (rev);
        }
        /* src_hostname */
        if (json_reader_read_member (rev, "src_hostname"))
        {
          g_string_append_printf (st, " src_hostname=\"%s\"", json_reader_get_string_value (rev));
          json_reader_end_element (rev);
        }
        /* dst_hostname */
        if (json_reader_read_member (rev, "dst_hostname"))
        {
          g_string_append_printf (st, " dst_hostname=\"%s\"", json_reader_get_string_value (rev));
          json_reader_end_element (rev);
        }
        /* src_mac */
        if (json_reader_read_member (rev, "src_mac"))
        {
          g_string_append_printf (st, " src_mac=\"%s\"", json_reader_get_string_value (rev));
          json_reader_end_element (rev);
        }
        /* dst_mac */
        if (json_reader_read_member (rev, "dst_mac"))
        {
          g_string_append_printf (st, " dst_mac=\"%s\"", json_reader_get_string_value (rev));
          json_reader_end_element (rev);
        }
        /* rep_prio_src */
        if (json_reader_read_member (rev, "rep_prio_src"))
        {
          g_string_append_printf (st, " rep_prio_src=\"%ld\"", json_reader_get_int_value (rev));
          json_reader_end_element (rev);
        }
        /* rep_prio_dst */
        if (json_reader_read_member (rev, "rep_prio_dst"))
        {
          g_string_append_printf (st, " rep_prio_dst=\"%ld\"", json_reader_get_int_value (rev));
          json_reader_end_element (rev);
        }
        /* rep_rel_src */
        if (json_reader_read_member (rev, "rep_rel_src"))
        {
          g_string_append_printf (st, " rep_rel_src=\"%ld\"", json_reader_get_int_value (rev));
          json_reader_end_element (rev);
        }
        /* rep_rel_dst */
        if (json_reader_read_member (rev, "rep_rel_dst"))
        {
          g_string_append_printf (st, " rep_rel_dst=\"%ld\"", json_reader_get_int_value (rev));
          json_reader_end_element (rev);
        }
        /* binary_data */
        if (json_reader_read_member (rev, "binary_data"))
        {
          g_string_append_printf (st, " binary_data=\"%s\"", json_reader_get_string_value (rev));
          json_reader_end_element (rev);
        }
        g_string_append_printf(st, "\n");
        result = g_string_free (st, FALSE);
 

        
 

    





    
      }
      json_reader_end_element (jr); 
    }
    g_object_unref (jr);
  }
  g_object_unref (jp);
  if (!result)
    g_string_free (st, TRUE); 
  return result;
}


// vim: set tabstop=2:

