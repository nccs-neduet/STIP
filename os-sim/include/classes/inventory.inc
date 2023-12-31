<?php
/**
* inventory.inc
*
* File Inventory.inc is used to:
*   - To manage inventory task
*
*
* License:
*
* Copyright (c) 2003-2006 ossim.net
* Copyright (c) 2007-2013 AlienVault
* All rights reserved.
*
* This package is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; version 2 dated June, 1991.
* You may not use, modify or distribute this program under any other version
* of the GNU General Public License.
*
* This package is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this package; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
* MA  02110-1301  USA
*
*
* On Debian GNU/Linux systems, the complete text of the GNU General
* Public License can be found in `/usr/share/common-licenses/GPL-2'.
*
* Otherwise you can read it here: http://www.gnu.org/licenses/gpl-2.0.txt
*
* @package    ossim-framework\Inventory
* @autor      AlienVault INC
* @license    http://www.gnu.org/licenses/gpl-2.0.txt
* @copyright  2003-2006 ossim.net
* @copyright  2007-2013 AlienVault
* @link       https://www.alienvault.com/
*/


/**
* Inventory Class
*
* Class for managing inventory tasks
*
* @package    ossim-framework\Assets
* @autor      AlienVault INC
* @copyright  2007-2013 AlienVault
* @link       https://www.alienvault.com/ AlienVault web page
*/


class Inventory
{
    private static $nmap_options = [
        //this options it is also known as aggressive option
        "privileged_mode"   => "--privileged",
        "unprivileged_mode" => "--unprivileged",
        "autodetect"        => "-O --osscan-guess --max-os-tries=1",
        "rdns"              => "-n",
        "scan_type_fast"    => "-sV -p21,22,23,25,53,80,113,115,135,139,161,389,443,445,554,1194,1241,1433,3000,3306,3389,8080,9390,27017",
        "scan_type_custom"  => "-sV -sS -p",
        "scan_type_normal"  => "-sV -sS",
        "scan_type_full"    => "-sV -sS -p1-65535",
        "scan_type_ping"    => "-sn -PE"
    ];

    //field task_type
    public static $inventory_task_type = [
        4 => "WMI Scan",
        5 => "Asset Discovery Scan"
    ];

    public static $wmi_scan = 4;
    //NMAP scan
    public static $asset_discovery = 5;


    /**
    * Class constructor
    *
    * This function sets up the class
    *
    */
    public function __construct()
    {
        ;
    }

    /**
     * This function will return the nmap options
     *
     * @param string   $key         key you are looking for. Example "scan_type_fast"
     * @param string   $extra       Extra parameter for the case of scan_type_custom.
     *
     * @return string the nmap required parameters
     */
    public static function get_nmap_options($key, $extra="")
    {
        if ($key == "scan_type_custom" && $extra == "") {
            return "";
        }

        if ($key != "scan_type_custom") {
            $extra = "";
        }

        if(!isset(static::$nmap_options[$key])){
            $option = "";
        }
        else {
            $option = static::$nmap_options[$key]." $extra";
        }
        return trim($option);
    }


    /**
    * This function returns a filtered inventory tasks
    *
    * @param object   $conn        Database access object
    * @param string   $sensor_id   [Optional] Sensor ID
    * @param string   $task_type   [Optional] Task type
    * @param string   $args        [Optional] SQL condition
    *
    * @throws Exception  If a connection error occurred
    *
    * @return array
    */
    public static function get_list($conn, $sensor_id = '', $task_type = '', $args = '')
    {
        Ossim_db::check_connection($conn);

        $list   = array();

        $params = array();
        $query  = "SELECT *, HEX(task_sensor) AS task_sensor FROM task_inventory WHERE 1";

        if ($sensor_id != '')
        {
            $query .= " AND task_sensor = UNHEX(?)";
            $params[] = $sensor_id;
        }

        if ($task_type != '')
        {
            $query .= " AND task_type = ?";
            $params[] = $task_type;
        }

        if ($args != '')
        {
            $query .= " $args ";
        }


        if (!$rs = $conn->Execute ($query, $params))
        {
            Av_exception::throw_error(Av_exception::DB_ERROR, $conn->ErrorMsg());
        }

        $iterator = 0;
        while (!$rs->EOF)
        {
            $list[] = array
            (
                "task_id"     => $rs->fields['task_id'],
                "task_type"   => $rs->fields['task_type'],
                "task_period" => $rs->fields['task_period'],
                "task_enable" => $rs->fields['task_enable'],
                "task_params" => $rs->fields['task_params'],
                "task_sensor" => $rs->fields['task_sensor'],
                "task_name"   => $rs->fields['task_name']
            );

            #nmap case
            if ($rs->fields['task_type'] == static::$asset_discovery){
                $list[$iterator]["nmap_extra_options"] = static::set_extra_nmap_options_from_task_params($rs->fields['task_params']);
            }
            $iterator++;
            $rs->MoveNext();
        }

        return $list;
    }

    /**
     * This function parses the task_params parameter and check if any matches with the Nmap parameters
     *
     * @param string   $task_params  String with nmap parameters
     *
     * @return array with the nmap extra options translated
     */
    public static function set_extra_nmap_options_from_task_params($task_params){
        $nmap_extra_options = [
            "ttemplate"          => "T3",
            "aggressive_scan"    => true,
            "rdns"               => true,
            "privileged_mode"    => true,
            "stype"              => "normal",
            "custom_scan_ports"  => "",
            "targets"            => ""
        ];

        if($task_params != '')
        {
            list($targets,$task_params) = Util::nmap_without_excludes($task_params);

            //targets
            $nmap_extra_options["targets"] = implode(", ",$targets);

            // Get timing template
            preg_match("/-(T[0-5])/", $task_params, $found);

            $nmap_extra_options["ttemplate"] = ($found[1] != '') ? $found[1]: '';

            // Aggressive scan - Autodetect services and Operating System
            preg_match("/(".static::get_nmap_options("autodetect").")/", $task_params, $found);
            $nmap_extra_options["aggressive_scan"] = (isset($found[1]) && $found[1] != '') ? TRUE : FALSE;

            // Reverse DNS resolution  -n (No DNS resolution)
            preg_match("/(".static::get_nmap_options("rdns").")/", $task_params, $found);
            $nmap_extra_options["rdns"] = (isset($found[1]) && $found[1] != '') ? FALSE : TRUE;

            // Privileged mode  --privileged (Privileged option will be set by default if not defined --unprivileged)
            preg_match("/(".static::get_nmap_options("unprivileged_mode").")/", $task_params, $found);
            $nmap_extra_options["privileged_mode"] = (isset($found[1]) && $found[1] != '') ? FALSE : TRUE;

            // Scan type
            if(preg_match("/".static::get_nmap_options("scan_type_fast")."/", $task_params))
            {
                $nmap_extra_options["stype"] = "fast";
            }
            elseif (preg_match("/".static::get_nmap_options("scan_type_full")."/", $task_params))
            {
                $nmap_extra_options["stype"] =  "full";
            }
            elseif (preg_match("/".static::get_nmap_options("scan_type_custom")."\s*?(\d+-\d+)/", $task_params, $found))
            {
                $nmap_extra_options["stype"] = "custom";
                $nmap_extra_options["custom_scan_ports"] = $found[1];
            }
            elseif (preg_match("/".static::get_nmap_options("scan_type_normal")."/", $task_params))
            {
                $nmap_extra_options["stype"] = "normal";
            }
            else
            {
                $nmap_extra_options["stype"] = "ping";
            }
        }

        return $nmap_extra_options;
    }


    /**
    * This function returns an inventory object
    *
    * @param object  $conn   Database access object
    * @param string  $id     Inventory ID
    *
    * @throws Exception  If a connection error occurred
    *
    * @return array
    */
    public static function get_object($conn, $id)
    {
        Ossim_db::check_connection($conn);

        $object = NULL;

        $query  = "SELECT *, HEX(task_sensor) as task_sensor FROM task_inventory WHERE task_id = ?";
        $params = array($id);

        $rs = $conn->Execute ($query, $params);
        if (!$rs)
        {
            Av_exception::throw_error(Av_exception::DB_ERROR, $conn->ErrorMsg());
        }

        if (!$rs->EOF)
        {
            $object = array(
                "task_id"     => $rs->fields['task_id'],
                "task_type"   => $rs->fields['task_type'],
                "task_period" => $rs->fields['task_period'],
                "task_enable" => $rs->fields['task_enable'],
                "task_params" => $rs->fields['task_params'],
                "task_sensor" => $rs->fields['task_sensor'],
                "task_name"   => $rs->fields['task_name']
            );

            #nmap case
            if ($rs->fields['task_type'] == static::$asset_discovery){
                $object["nmap_extra_options"] = static::set_extra_nmap_options_from_task_params($rs->fields['task_params']);
            }
        }

        return $object;
    }


    /**
    * This function inserts a new inventory task
    *
    * @param object   $conn          Database access object
    * @param string   $sensor_id     Sensor ID
    * @param string   $task_name     Task name
    * @param string   $task_type     Task type
    * @param string   $task_params   Task params
    * @param string   $nets          [Optional] Nets to scan
    *
    * @throws Exception  If a connection error occurred
    *
    * @return integer
    */
    public static function insert($conn, $sensor_id, $task_name, $task_type, $task_period, $task_params, $nets = '')
    {
        Ossim_db::check_connection($conn);

        $last_id = -1;

        $task_targets = self::translate_cidr_sensor_to_id($conn, $nets, $sensor_id);

        $query = "INSERT INTO task_inventory (task_sensor, task_name, task_type, task_period, task_enable, task_params, task_targets)
            VALUES (UNHEX(?), ?, ?, ?, ?, ?, ?)";

        $params = array(
            $sensor_id,
            $task_name,
            $task_type,
            $task_period,
            1,
            $task_params,
            $task_targets
        );

        $rs = $conn->Execute($query, $params);

        if (!$rs)
        {
            Av_exception::throw_error(Av_exception::DB_ERROR, $conn->ErrorMsg());
        }

        $query = ossim_query('SELECT LAST_INSERT_ID() AS id FROM task_inventory');

        $rs = $conn->Execute($query);

        if (!$rs)
        {
           Av_exception::throw_error(Av_exception::DB_ERROR, $conn->ErrorMsg());
        }
        else
        {
            $last_id = $rs->fields['id'];
        }

        self::refresh();

        return $last_id;
    }


    /**
    * This function updates an inventory task
    *
    * @param object   $conn          Database access object
    * @param string   $task_id       Task ID
    * @param string   $sensor_id     Sensor ID
    * @param string   $task_name     Task name
    * @param string   $task_type     Task type
    * @param string   $task_params   Task params
    * @param string   $nets          Nets to scan
    *
    * @throws Exception  If a connection error occurred
    *
    * @return boolean
    */
    public static function modify($conn, $task_id, $sensor_id, $task_name, $task_type, $task_period, $task_params, $nets)
    {
        Ossim_db::check_connection($conn);

        $task_targets = self::translate_cidr_sensor_to_id($conn, $nets, $sensor_id);

        $query        = "UPDATE task_inventory
                         SET task_sensor = UNHEX(?), task_name = ?, task_type = ?, task_period = ?, task_params = ?, task_targets = ?
                         WHERE task_id = ?";

        $params = array(
            $sensor_id,
            $task_name,
            $task_type,
            $task_period,
            $task_params,
            $task_targets,
            $task_id
        );

        $rs = $conn->Execute($query, $params);

        if (!$rs)
        {
            Av_exception::throw_error(Av_exception::DB_ERROR, $conn->ErrorMsg());
        }

        self::refresh();

        return TRUE;
    }


    /**
     * This function enables or disables a scheduled scan
     *
     * @param object   $conn      Database access object
     * @param string   $task_id   Task ID
     *
     * @throws Exception  If a connection error occurred
     *
     * @return boolean
     */
    public static function toggle_scan($conn, $task_id)
    {
        Ossim_db::check_connection($conn);

        $query  = "UPDATE task_inventory SET task_enable = NOT(task_enable) WHERE task_id = ?";
        $params = array($task_id);

        $rs = $conn->Execute($query, $params);

        if (!$rs)
        {
            Av_exception::throw_error(Av_exception::DB_ERROR, $conn->ErrorMsg());
        }

        self::refresh();

        return TRUE;
    }


    /**
    * This function deletes an inventory task
    *
    * @param object   $conn      Database access object
    * @param string   $task_id   Task ID
    *
    * @throws Exception  If a connection error occurred
    *
    * @return boolean
    */
    public static function delete($conn, $task_id)
    {
        Ossim_db::check_connection($conn);

        $query  = "DELETE FROM task_inventory WHERE task_id = ?";
        $params = array($task_id);

        $rs = $conn->Execute($query, $params);

        if (!$rs)
        {
            Av_exception::throw_error(Av_exception::DB_ERROR, $conn->ErrorMsg());
        }

        self::refresh();

        return TRUE;
    }


    /**
    * This function returns the net IDs related to CIDRs and sensor
    *
    * @param object   $conn     Database access object
    * @param string   $cidrs    Net CIDRs separated by comma
    * @param string   $sensor   Sensor ID
    *
    * @throws Exception  If a connection error occurred
    *
    * @return string
    */
    private static function translate_cidr_sensor_to_id($conn, $cidrs, $sensor)
    {
        Ossim_db::check_connection($conn);

        $nets   = array();
        $perms  = (Session::get_ctx_where() != '') ? " AND n.ctx in (".Session::get_ctx_where().")" : '';
        $perms .= (Session::get_net_where() != '') ? " AND n.id in (".Session::get_net_where().")" : '';

        $cidrs  = explode(' ', $cidrs);

        if(is_array($cidrs) && !empty($cidrs))
        {
            foreach($cidrs as $cidr)
            {
                $query = "SELECT HEX(n.id) AS id
                           FROM net n, net_cidrs nc, net_sensor_reference ns
                           WHERE n.id = nc.net_id AND n.id=ns.net_id AND ns.sensor_id = UNHEX(?) AND nc.cidr = ? $perms";


                $params =  array($sensor, $cidr);

                $rs = $conn->Execute($query, $params);

                if (!$rs)
                {
                    Av_exception::throw_error(Av_exception::DB_ERROR, $conn->ErrorMsg());
                }

                while (!$rs->EOF)
                {
                    $nets[$rs->fields['id']] = $rs->fields['id'];

                    $rs->MoveNext();
                }
            }
        }

        return implode(' ', $nets);
    }


    /**
    * This function send refresh signal to frameworkd
    *
    * @return boolean
    */
    public static function refresh()
    {
        // Refresh cache
        Util::memcacheFlush();

        try
        {
            $frcon = new Frameworkd_socket();
            $frcon->write("control action=\"refresh_inventory_task\"\n");
        }
        catch(Exception $e)
        {
            return FALSE;
        }

        return TRUE;
    }
}

/* End of file inventory.inc */
/* Location: ../include/classes/inventory.inc */
