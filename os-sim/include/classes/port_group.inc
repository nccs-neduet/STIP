<?php
/**
 * Port_group.inc
 *
 * File Port_group.inc is used to:
 *   - Show the port groups
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
 * @package    ossim-framework\Assets
 * @autor      AlienVault INC
 * @license    http://www.gnu.org/licenses/gpl-2.0.txt
 * @copyright  2003-2006 ossim.net
 * @copyright  2007-2013 AlienVault
 * @link       https://www.alienvault.com/
 */


class Port_group
{
    public $id;

    public $name;

    public $descr;

    public $ctx;

    public $foundrows;

    public function __construct($id, $name, $descr, $ctx, $foundrows = 0)
    {
        $this->id = $id;
        $this->name = $name;
        $this->descr = $descr;
        $this->ctx = $ctx;
        $this->foundrows = $foundrows;
    }
    

    public function get_id()
    {
        return $this->id;
    }


    public function get_name()
    {
        return $this->name;
    }

    public function get_descr($entities = TRUE)
    {
        if ($entities)
        {
            return Util::htmlentities($this->descr);
        }
        else
        {
            return $this->descr;
        }
    }


    public function get_ctx()
    {
        return $this->ctx;
    }


    public function get_foundrows()
    {
        return $this->foundrows;
    }


    public function get_reference_ports($conn, $port_group_id)
    {
        return Port_group_reference::get_list($conn, "WHERE port_group_id = $port_group_id");
    }


    public static function get_list($conn, $args = "")
    {
        $list = array();

        $iffoundrows = preg_match("/LIMIT/i", $args) ? TRUE : FALSE;

        $select = ($iffoundrows) ? "SQL_CALC_FOUND_ROWS *, HEX(ctx) AS ctx" : "*, HEX(ctx) AS ctx";
        $query  = ossim_query("SELECT $select FROM port_group $args");

        $rs = $conn->Execute($query);
        if (!$rs)
        {
            Av_exception::write_log(Av_exception::DB_ERROR, $conn->ErrorMsg());
        }
        else
        {
            $foundrows = 0;
            if ($iffoundrows)
            {
                $rf = $conn->Execute("SELECT FOUND_ROWS() as total");

                if (!$rf)
                {
                    Av_exception::write_log(Av_exception::DB_ERROR, $conn->ErrorMsg());
                }
                else
                {
                    $foundrows = $rf->fields["total"];
                }
            }

            while (!$rs->EOF)
            {
                $list[] = new self($rs->fields["id"], $rs->fields["name"], $rs->fields["descr"], $rs->fields["ctx"], $foundrows);

                $rs->MoveNext();
            }
        }

        return $list;
    }


    public static function insert($conn, $name, $port_list, $descr, $ctx)
    {
        $query = "INSERT INTO port_group (name, descr, ctx) VALUES (?, ?, UNHEX(?))";
        $params = array($name, $descr, $ctx);

        if ($conn->Execute($query, $params) === FALSE)
        {
            Av_exception::throw_error(Av_exception::DB_ERROR, $conn->ErrorMsg());
        }

        $query = ossim_query("SELECT LAST_INSERT_ID() as newid");

        $rs = $conn->Execute($query);
        if (!$rs)
        {
            Av_exception::throw_error(Av_exception::DB_ERROR, $conn->ErrorMsg());
        }
        else
        {
            $new_id = $rs->fields['newid'];
        }

        Port_group_reference::insert($conn, $new_id, $port_list, $ctx);

        $infolog = array($name, "(" . $descr . ")");
        Log_action::log(39, $infolog);

        return $new_id;
    }


    public static function update($conn, $id, $name, $port_list, $descr, $ctx)
    {
        $query = "UPDATE port_group SET name = ?, descr = ?, ctx = UNHEX(?) WHERE id = ?";

        $params = array($name, $descr, $ctx, $id);

        if ($conn->Execute($query, $params) === FALSE)
        {
            Av_exception::throw_error(Av_exception::DB_ERROR, $conn->ErrorMsg());
        }

        Port_group_reference::delete($conn, $id);
        Port_group_reference::insert($conn, $id, $port_list, $ctx);

        $infolog = array($name, "(" . $descr . ")");

        Log_action::log(40, $infolog);
    }


    public static function delete($conn, $id)
    {
        $query = "DELETE FROM port_group WHERE id = ?";
        $params = array($id);

        if ($conn->Execute($query, $params) === FALSE)
        {
            Av_exception::throw_error(Av_exception::DB_ERROR, $conn->ErrorMsg());
        }

        Port_group_reference::delete($conn, $id);

        $infolog = array($id);
        Log_action::log(41, $infolog);
    }


    public static function can_delete($conn, $id)
    {
        $query = "SELECT count(*) as num FROM policy_port_reference WHERE port_group_id = ?";
        $params = array($id);

        $rs = $conn->Execute($query, $params);
        if (!$rs)
        {
            Av_exception::throw_error(Av_exception::DB_ERROR, $conn->ErrorMsg());
        }

        if ($rs->fields['num'] == '0')
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }


    public static function get_name_by_id($conn, $id)
    {
        $query = "SELECT name FROM port_group WHERE id = ?";
        $params = array($id);

        $rs = $conn->Execute($query, $params);

        if (!$rs)
        {
            return _('Unknonwn');
        }
        else
        {
            return (empty($rs->fields["name"]) ?  _('Unknonwn') : $rs->fields["name"]);
        }
    }

    public static function expand_ports($actives_ports)
    {
        $aux = array();

        foreach($actives_ports as $active_port)
        {
            // expand port range (100-102 - tcp) to port list (100 - tcp, 101 -tcp, 102 - tcp)
            if (preg_match('/(\d+)-(\d+)\s*\-\s*(\w+)/', $active_port, $found))
            {
                $ports = range($found[1], $found[2]);

                $aux = [];
                foreach($ports as $p_key => $p_value){
                    $aux[] = security_class::append_protocol($p_value, $p_key, $found[3]);
                }
                $ports = $aux;

                // delete element in the array
                $key = array_search($active_port, $actives_ports);

                unset($actives_ports[$key]);

                // append the returned ports
                $aux = array_merge($aux, $ports);
            }
        }

        $actives_ports = array_merge($actives_ports, $aux);
        $actives_ports = array_unique($actives_ports);

        return $actives_ports;
    }

    public static function group_ports($actives_ports)
    {
        $tcp_list = array();
        $udp_list = array();

        foreach($actives_ports as $v)
        {
            // group port list (100 - tcp, 101 -tcp, 102 - tcp) to port group (100-102 - tcp)

            $delete_item = FALSE;

            if ( preg_match('/(\d+)\s*\-\s*tcp/i', trim($v)) )
            {
                $tcp_list[] = preg_replace('/[^\d]/', '', $v);
                $delete_item = TRUE;
            }
            else if ( preg_match('/(\d+)\s*\-\s*udp/i', trim($v)) )
            {
                $udp_list[] = preg_replace('/[^\d]/', '', $v);
                $delete_item = TRUE;
            }

            if ($delete_item === TRUE)
            {
                $key = array_search($v, $actives_ports);
                unset($actives_ports[$key]);
            }
        }

        if (!empty($tcp_list))
        {
            sort($tcp_list ,SORT_NUMERIC);
            $tcp_list = Util::group_nums($tcp_list);

            $aux = [];
            foreach($tcp_list as $p_key => $p_value){
                $aux[] = security_class::append_protocol($p_value, $p_key, 'tcp');
            }
            $tcp_list = $aux;
        }

        if (!empty($udp_list))
        {
            sort($udp_list ,SORT_NUMERIC);
            $udp_list = Util::group_nums($udp_list);

            $aux = [];
            foreach($udp_list as $p_key => $p_value){
                $aux[] = security_class::append_protocol($p_value, $p_key, 'udp');
            }
            $udp_list = $aux;
        }

        $actives_ports = array_merge($actives_ports, $tcp_list, $udp_list);

        return $actives_ports;
    }
}

/* End of file port_group.inc */
/* Location: ../include/classes/port_group.inc */
