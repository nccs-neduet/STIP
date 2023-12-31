<?php
/**
* ossec.inc
*
* File ossec.inc is used to:
*   - To manage OSSEC rules
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
* @package    ossim-framework\Ossec
* @autor      AlienVault INC
* @license    http://www.gnu.org/licenses/gpl-2.0.txt
* @copyright  2003-2006 ossim.net
* @copyright  2007-2013 AlienVault
* @link       https://www.alienvault.com/
*/


/**
* Ossec Class
*
* Class for managing OSSEC rules
*
* @package    ossim-framework\Ossec
* @autor      AlienVault INC
* @copyright  2007-2013 AlienVault
* @link       https://www.alienvault.com/ AlienVault web page
*/

class Ossec
{
    /**
     * Rules path
     *
     */
    const RULES_PATH = 'ossec/rules';

    /**
     * Configuration path
     *
     */
    const CONF_PATH  = 'ossec/ossec.conf';


    /**
     * Class constructor
     *
     * This function sets up the class
     */
    public function __construct()
    {

    }


    /**
     * This function gets rule file content
     *
     * @param string   $sensor_id   Sensor ID (Table alienvault.sensor)
     * @param boolean  $rule_file   Rule file name
     *
     * @throws Exception  If an error occurred
     *
     * @return array
     */
    public static function get_rule_file($sensor_id, $rule_file)
    {
        $data = array();

        $alienvault_conn = new Alienvault_conn();
        $provider_registry = new Provider_registry();
        $client = new Alienvault_client($alienvault_conn, $provider_registry);
        $response = $client->sensor($sensor_id)->get_ossec_rule_file($rule_file);
        $response = @json_decode($response, TRUE);

        if (!$response || $response['status'] == 'error')
        {
            $exp_msg = $client->get_error_message($response);

            Av_exception::throw_error(Av_exception::USER_ERROR, $exp_msg);
        }

        $conf_file = $response['data']['local_path'];

        $data['path'] = $conf_file;
        $data['data'] = @file_get_contents($conf_file);

        if ($data['data'] === FALSE)
        {
            $exp_msg = _("Error! Failed to get $rule_file. You do not have enough access privileges");

            Av_exception::throw_error(Av_exception::USER_ERROR, $exp_msg);
        }

        return $data;
    }


    /**
     * This function sets new content for a rule file
     *
     * @param string  $sensor_id   Sensor ID (Table alienvault.sensor)
     * @param string  $file        Rule file (local_rules.xml or rules_config.xml)
     * @param string  $new_conf    New data
     *
     * @throws Exception  If an error occurred
     *
     * @return array
     */
    public static function set_rule_file($sensor_id, $file, $new_conf)
    {
        $data['status']  = 'success';
        $data['msg']     = _("$file file updated successfully");
        $data['data']    = $new_conf;

        if (!self::is_editable($file))
        {
            $exp_msg = _('Error! Failed to set rule file. File name is empty or not editable');

            Av_exception::throw_error(Av_exception::USER_ERROR, $exp_msg);
        }

        $rule_path = Ossec_utilities::get_real_path($sensor_id, self::RULES_PATH.'/'.$file);

        if (@file_put_contents($rule_path, $new_conf, LOCK_EX) === FALSE)
        {
            $exp_msg = _("Error! Failed to set $file. You do not have enough access privileges");

            Av_exception::throw_error(Av_exception::USER_ERROR, $exp_msg);
        }

        $alienvault_conn = new Alienvault_conn();
        $provider_registry = new Provider_registry();
        $client = new Alienvault_client($alienvault_conn, $provider_registry);
        $response = $client->sensor($sensor_id)->set_rule_file($file);
        $response = @json_decode($response, TRUE);

        if (!$response || $response['status'] == 'error')
        {
            $exp_msg = $client->get_error_message($response);

            Av_exception::throw_error(Av_exception::USER_ERROR, $exp_msg);
        }

        return $data;
    }


    /**
     * This function gets the OSSEC configuration (ossec.conf)
     *
     * @param string   $sensor_id   Sensor ID (Table alienvault.sensor)
     *
     * @throws Exception  If an error occurred
     *
     * @return array
     */
    public static function get_configuration_file($sensor_id)
    {
        $data = array();

        $alienvault_conn = new Alienvault_conn();
        $provider_registry = new Provider_registry();
        $client = new Alienvault_client($alienvault_conn, $provider_registry);
        $response = $client->sensor($sensor_id)->get_ossec_config();
        $response = @json_decode($response, TRUE);

        if (!$response || $response['status'] == 'error')
        {
            $exp_msg = $client->get_error_message($response);

            Av_exception::throw_error(Av_exception::USER_ERROR, $exp_msg);
        }

        $conf_file = $response['data']['local_path'];

        $data['path'] = $conf_file;
        $data['data'] = @file_get_contents($conf_file);

        if ($data['data'] === FALSE)
        {
            $exp_msg = _('Error! Failed to get the HIDS configuration file. You do not have enough access privileges');

            Av_exception::throw_error(Av_exception::USER_ERROR, $exp_msg);
        }

        return $data;
    }


    /**
     * Given a OSSEC configuration string it will validate some basic structure required to work within AV
     *
     * @param string  $new_conf    New configuration
     *
     * @return array
     * [
     *      status => "success|warning"
     *      msg => STRING
     *      data => STRING with the final configuration
     * ]
     */

    public static function validate_configuration_file($new_conf) {
        $data = [
            'status' => 'success',
            'msg' =>_('HIDS configuration file updated successfully')
        ];

        $warning_msg = "";

        //Special case 0:  global section
        $pattern = '/<\s*ossec_config\s*>.*<\s*\/ossec_config\s*>/ms';
        if (!preg_match($pattern, $new_conf))
        {
            $new_conf = substr_replace($new_conf, "<ossec_config></ossec_config>", 0, 0);
        }

        $pattern = '/<\s*global\s*>.*<\s*\/global\s*>/ms';
        if (!preg_match($pattern, $new_conf))
        {
            $pos = strpos($new_conf, "<ossec_config>") + strlen( "<ossec_config>");
            $pos = $pos === FALSE ? 0: $pos;//if no global pressent it will be added to the begining => and the file will fail it configuration
            $new_conf = substr_replace($new_conf, "\n<global></global>", $pos, 0);
        }


        //Special case 1:  Disable email notification
        $pattern_full = '/<\s*email_notification\s*>\s*no\s*<\s*\/email_notification\s*>/';
        $pattern = '/<\s*email_notification\s*>[^<]*<\s*\/email_notification\s*>/';

        if (!preg_match($pattern_full, $new_conf)){
            if (preg_match($pattern, $new_conf))
            {
                $new_conf = preg_replace($pattern, '<email_notification>no</email_notification>', $new_conf);
            }
            else {
                $pos = strpos($new_conf, "</global>");
                $pos = $pos === FALSE ? 0: $pos;//if no global pressent it will be added to the begining => and the file will fail it configuration
                $new_conf = substr_replace($new_conf, "\n<email_notification>no</email_notification>\n", $pos, 0);
            }

            $system_name = (Session::is_pro()) ? 'AlienVault' : 'OSSIM';
            $warning_msg .= '<div style="padding: 3px 0px 3px 15px;">'._('HIDS email notification is not supported. Please, use '.$system_name.' email notifications instead').'.</div>';
        }

        //Special case 2:  Disable database configuration
        $pattern = '/\s*<\s*database_output\s*>[^<]*<\s*\/database_output\s*>/s';

        if (preg_match($pattern, $new_conf))
        {
            $new_conf = preg_replace($pattern, '', $new_conf);

            $warning_msg .= '<div style="padding: 3px 0px 3px 15px;">'._('HIDS not compiled with support for \'mysql\'').'.</div>';
        }

        //Special case 3:  custom_alert_output
        $pattern_full = "/\s*<\s*custom_alert_output\s*>AV - Alert - \"\\\$TIMESTAMP\" --> RID: \"\\\$RULEID\"; RL: \"\\\$RULELEVEL\"; RG: \"\\\$RULEGROUP\"; RC: \"\\\$RULECOMMENT\"; USER: \"\\\$DSTUSER\"; SRCIP: \"\\\$SRCIP\"; HOSTNAME: \"\\\$HOSTNAME\"; LOCATION: \"\\\$LOCATION\"; EVENT: \"\[INIT\]\\\$FULLLOG\[END\]\";\s*<\s*\/custom_alert_output\s*>\s*/s";
        $pattern = "/<\s*custom_alert_output\s*>[^<]*<\s*\/custom_alert_output\s*>/s";

        if (!preg_match($pattern_full, $new_conf)) {
            $warning_msg .= '<div style="padding: 3px 0px 3px 15px;">'._('HIDS not configured with AV output support');
            $custom_alert_output = "<custom_alert_output>AV - Alert - \"\$TIMESTAMP\" --> RID: \"\$RULEID\"; RL: \"\$RULELEVEL\"; RG: \"\$RULEGROUP\"; RC: \"\$RULECOMMENT\"; USER: \"\$DSTUSER\"; SRCIP: \"\$SRCIP\"; HOSTNAME: \"\$HOSTNAME\"; LOCATION: \"\$LOCATION\"; EVENT: \"[INIT]\$FULLLOG[END]\"; </custom_alert_output>";
            if (preg_match($pattern, $new_conf)) {
                $warning_msg .= '. Other configuration detected, setting AV configuration</div>';
                $new_conf = preg_replace($pattern, $custom_alert_output, $new_conf);
            }
            else{
                $warning_msg .= '. No configuration detected, setting AV configuration</div>';
                $pos = strpos($new_conf, "</global>");
                $pos = $pos === FALSE ? 0: $pos;//if no global pressent it will be added to the begining => and the file will fail it configuration
                $new_conf = substr_replace($new_conf, $custom_alert_output."\n", $pos, 0);

            }
            $warning_msg .= '.</div>';
        }

        $data['data'] = $new_conf;

        if ( $warning_msg != ""){
            $data['msg'] = _('HIDS configuration file updated with warnings:').'<br/>' . $warning_msg;
            $data['status'] = 'warning';
        }

        return $data;
    }


    /**
     * Given a string with an ossec configuration it would be stored in the sensor if it is a valid one
     *
     * @param string  $sensor_id   Sensor ID (Table alienvault.sensor)
     * @param string  $new_conf    New configuration
     *
     * @throws Exception  If an error occurred
     *
     *
     */

    public static function set_remote_configuration_file($sensor_id, $new_conf){
        $conf_file = Ossec_utilities::get_real_path($sensor_id, self::CONF_PATH);

        if (file_put_contents($conf_file, $new_conf, LOCK_EX) === FALSE)
        {
            $exp_msg = _('Error! Failed to set HIDS configuration file. You do not have enough access privileges');

            Av_exception::throw_error(Av_exception::USER_ERROR, $exp_msg);
        }

        $alienvault_conn = new Alienvault_conn();
        $provider_registry = new Provider_registry();
        $client = new Alienvault_client($alienvault_conn, $provider_registry);
        $response = $client->sensor($sensor_id)->set_ossec_config();
        $response = @json_decode($response, TRUE);


        if (!$response || $response['status'] == 'error')
        {
            $exp_msg = $client->get_error_message($response);

            Av_exception::throw_error(Av_exception::USER_ERROR, $exp_msg);
        }
    }

    /**
     * This function sets the new OSSEC configuration (ossec.conf)
     *
     * @param string  $sensor_id   Sensor ID (Table alienvault.sensor)
     * @param string  $new_conf    New configuration
     *
     * @throws Exception  If an error occurred
     *
     * @return array
     * [
     *      status => "success|warning"
     *      msg => STRING
     *      data => STRING with the final configuration
     * ]
     */

    public static function set_configuration_file($sensor_id, $new_conf)
    {

        $data = static::validate_configuration_file($new_conf);

        self::set_remote_configuration_file($sensor_id, $data['data']);

        $user_name = Session::get_session_user();
        Log_action::log(101,[$user_name, $data['msg']]);

        return $data;
    }


    /**
     * This function checks if a rule file is editable file
     *
     * @param string  $rule_file  Rule file name
     * @param string  $new_conf   New configuration
     *
     * @return boolean
     */
    public static function is_editable($rule_file)
    {
        $e_rule_files = array(
            'local_rules.xml'  => 1,
            'rules_config.xml' => 1
        );

        if (!empty($rule_file) && (!empty($e_rule_files[$rule_file]) || preg_match('/^av_/', $rule_file)))
        {
            return TRUE;
        }

        return FALSE;
    }


    /**
     * This function returns the order for all rule files
     *
     * @param string  $sensor_id   Sensor ID (Table alienvault.sensor)
     *
     * @return array
     */
    public static function get_rule_order($sensor_id)
    {
        $conf_file = Ossec_utilities::get_real_path($sensor_id, self::CONF_PATH);

        $rule_order = array(
			'rules_config.xml' => 1,
			'pam_rules.xml' => 2,
			'sshd_rules.xml' => 3,
			'telnetd_rules.xml' => 4,
			'syslog_rules.xml' => 5,
			'arpwatch_rules.xml' => 6,
			'symantec-av_rules.xml' => 7,
			'symantec-ws_rules.xml' => 8,
			'pix_rules.xml' => 9,
			'named_rules.xml' => 10,
			'smbd_rules.xml' => 11,
			'vsftpd_rules.xml' => 12,
			'pure-ftpd_rules.xml' => 13,
			'proftpd_rules.xml' => 14,
			'ms_ftpd_rules.xml' => 15,
			'ftpd_rules.xml' => 16,
			'hordeimp_rules.xml' => 17,
			'vpopmail_rules.xml' => 18,
			'vmpop3d_rules.xml' => 19,
			'courier_rules.xml' => 20,
			'web_rules.xml' => 21,
			'apache_rules.xml' => 22,
			'mysql_rules.xml' => 23,
			'postgresql_rules.xml' => 24,
			'ids_rules.xml' => 25,
			'squid_rules.xml' => 26,
			'firewall_rules.xml' => 27,
			'cisco-ios_rules.xml' => 28,
			'netscreenfw_rules.xml' => 29,
			'sonicwall_rules.xml' => 30,
			'postfix_rules.xml' => 31,
			'sendmail_rules.xml' => 32,
			'imapd_rules.xml' => 33,
			'mailscanner_rules.xml' => 34,
			'ms-exchange_rules.xml' => 35,
			'racoon_rules.xml' => 36,
			'vpn_concentrator_rules.xml' => 37,
			'spamd_rules.xml' => 38,
			'msauth_rules.xml' => 39,
			'mcafee_av_rules.xml' => 40,
			'roundcube_rules.xml' => 41,
			'dropbear_rules.xml' => 42,
			'dovecot_rules.xml' => 43,
			'ms_dhcp_rules.xml' => 44,
			'cimserver_rules.xml' => 45,
			'attack_rules.xml' => 46,
			'wordpress_rules.xml' => 47,
			'asterisk_rules.xml' => 48,
			'nginx_rules.xml' => 49,
			'vmware_rules.xml' => 50,
			'ossec_rules.xml' => 51,
			'zeus_rules.xml' => 52,
			'solaris_bsm_rules.xml' => 53,
			'clam_av_rules.xml' => 54,
			'trend-osce_rules.xml' => 55,
			'openbsd_rules.xml' => 56,
			'web_appsec_rules.xml' => 57,
			'ms-se_rules.xml' => 58,
			'php_rules.xml' => 59,
			'unbound_rules.xml' => 60,
			'sysmon_rules.xml' => 61,
			'opensmtpd_rules.xml' => 62,
			'apparmor_rules.xml' => 63,
			'systemd_rules.xml' => 64,
			'firewalld_rules.xml' => 65,
            'local_rules.xml' => -1
       );

        if (@file_exists($conf_file) !== FALSE)
        {
            $pattern = "<[[:space:]]*include[[:space:]]*>av_.*<[[:space:]]*/[[:space:]]*include[[:space:]]*>";

            $rules = Util::execute_command("egrep ? ?", array($pattern, $conf_file), $rules, 'array');

            if (is_array($rules) && !empty($rules))
            {
                foreach ($rules as $v)
                {
                    $pattern = array('/<[[:space:]]*include[[:space:]]*>/', '/<[[:space:]]*\/[[:space:]]*include[[:space:]]*>/');
                    $replace = array ('', '');
                    $rule    = preg_replace($pattern, $replace, $v);
                    $size    = count($rule_order) + 1;

                    $rule_order[trim($rule)] = $size;
                }
            }
        }

        return $rule_order;
    }


    /**
     * This function returns all rule files available
     *
     * @param string   $sensor_id   Sensor ID (Table alienvault.sensor)
     * @param boolean  $cache       Use cached information
     *
     * @throws Exception  If an error occurred
     *
     * @return array
     */
    public static function get_rule_files($sensor_id, $cache = TRUE)
    {
        $rules = array();

        $file = 'ossec_rules_'.$sensor_id;

        if ($cache == TRUE)
        {
            $rules = Cache_file::get_data($file, 3600);
        }

        //Cache has expired or there is no cache information
        if (empty($rules))
        {
            $alienvault_conn = new Alienvault_conn();
            $provider_registry = new Provider_registry();
            $client = new Alienvault_client($alienvault_conn, $provider_registry);
            $response = $client->sensor($sensor_id)->get_ossec_rule_files();
            $response = @json_decode($response, TRUE);

            if (!$response || $response['status'] == 'error')
            {
                $exp_msg = $client->get_error_message($response);

                Av_exception::throw_error(Av_exception::USER_ERROR, $exp_msg);
            }

            $rules = $response['data']['rules'];

            Cache_file::save_file($file, $rules);
        }

        return $rules;
    }


    /*********************************
    ************** Tree **************
    **********************************/

    public static function get_tree($sensor_id, $file)
    {
        $tree = NULL;

        if (preg_match("/[^\_\-a-zA-Z0-9\.]+/", $file))
        {
            $exp_msg = _('Error! Failed to get XML file. Invalid file name');

            Av_exception::throw_error(Av_exception::USER_ERROR, $exp_msg);
        }

        // Rule file
        $rule_data = self::get_rule_file($sensor_id, $file);

        $lk_name = self::get_key_name($file);

        $xml_obj = new Xml_parser($lk_name);
        $xml_obj->load_string($rule_data['data']);

        if($xml_obj->errors['status'] == FALSE)
        {
            $exp_msg = _('Error! Failed to parse XML file:').implode('', $xml_obj->errors['msg']);

            Av_exception::throw_error(Av_exception::USER_ERROR, $exp_msg);
        }

        $tree = $xml_obj->xml2array();

        return $tree;
    }


    public static function get_child($tree, $lk_name, $lk_value, $name_node = '', $parents = '')
    {
        $at_key = (preg_match("/^attr_/", $lk_value) != FALSE)  ? TRUE : FALSE;

        if ($name_node !== '')
        {
            $parents[] = "'".$name_node."'";
        }

        if (is_array($tree))
        {
            if (isset($tree['@attributes']))
            {
                $lk_value_2 = ($at_key == TRUE) ? preg_replace("/^attr_/", '', $lk_value) : $lk_value;

                if ($tree['@attributes'][$lk_name] === $lk_value_2)
                {
                    $tree = ($at_key == TRUE) ? $tree['@attributes'] : $tree;

                    $name_node = ($at_key == TRUE) ? "@at_".$name_node : $name_node;

                    return array ('node' => $name_node, 'tree' => $tree, 'parents' => $parents);
                }
            }

            foreach ($tree as $k => $children)
            {
                if ($k !== '@attributes')
                {
                    $found = self::get_child($children, $lk_name, $lk_value, $k, $parents);

                    if (!empty($found))
                    {
                        return $found;
                    }
                }
            }

        }
    }

    /*
    * Types:
    *   [1]  Attribute
    *   [2]  Attributes
    *   [3]  Text Node
    *   [4]  Node with level <=2
    *   [5]  Node with level > 2
    */

    public static function get_node_type($node_name, $node)
    {
        if (isset($node['tree']['@attributes'][$node_name]))
        {
            return 1;
        }
        else if ($node_name == _('Attributes'))
        {
            return 2;
        }
        elseif (!isset ($node['tree']['@attributes'][$node_name]) && !is_array($node['tree'][0]))
        {
            return 3;
        }
        elseif (self::get_level($node['tree']) <= 2)
        {
            return 4;
        }
        else
        {
            return 5;
        }
    }


    public static function get_level($tree, $level = '0')
    {
        $max_level = 0;

        if (is_array ($tree))
        {
            $level = $level + 1;

            foreach ($tree as $k => $children)
            {
                if ($k !== '@attributes')
                {
                    $nl = self::get_level($children, $level);
                    $max_level = ($max_level > $nl) ? $max_level : $nl;
                }
            }

            return $max_level;
        }

        return $level-1;
    }


    public static function set_new_lk($tree, $lk_name, $lk_value, $new_lk_value)
    {
        // Patch regex DoS possible vuln
        $lk_value = Util::regex($lk_value);

        $pattern = "/$lk_value/";

        if (is_array($tree))
        {
            foreach ($tree as $k => $v)
            {
                if ($k === '@attributes')
                {
                    $tree[$k][$lk_name] = preg_replace($pattern, $new_lk_value, $v[$lk_name]);
                }
                else
                {
                    $tree[$k] = self::set_new_lk($tree[$k], $lk_name, $lk_value, $new_lk_value);
                }
            }
        }

        return $tree;
    }


     /**
     * This function returns a unique key which is necessary to parse XML file
     *
     * @param string   $file  XML file
     *
     * @return string
     */
    public static function get_key_name($file)
    {
        $lk_name = 'av_key_'.md5($file);

        return $lk_name;
    }


    public static function get_ac_type($parents)
    {
        $pattern = "/^'?var|group|rule'?$/";
        $size    = count($parents);

        for ($i = $size-1; $i >= 0; $i--)
        {
            if (preg_match($pattern, $parents[$i]))
            {
                $res = array(
                    'parent'       => preg_replace("/'/", '', $parents[$i]),
                    'current_node' => preg_replace("/'/", '', $parents[$size-1])
                );

                return $res;
            }
        }
    }


    public static function get_nodes($tree, $node_name)
    {
        $nodes = NULL;

        if (is_array($tree))
        {
            foreach ($tree as $k => $v)
            {
                if (is_array($v) && $k === $node_name)
                {
                    $nodes[] = $v;
                }
                else if(is_array($v) && $k !== $node_name)
                {
                    $aux     = self::get_nodes($v, $node_name);
                    $nodes   = (is_array($nodes)) ? $nodes : array();
                    $aux     = (is_array($aux)) ? $aux : array();
                    $nodes   = array_merge ($nodes, $aux);
                }
            }
        }

        return $nodes;
    }


    public static function get_icon($key, $item, $lk_name)
    {
        $icon = NULL;

        $lk_value = $item['@attributes'][$lk_name];

        $depth_level = count(explode('_', $lk_value));

        if ($depth_level == 2)
        {
            if ($key == 'group')
            {
                $icon = 'ruler-triangle.png';
            }
            else if ($key == 'var')
            {
                $icon = 'switch.png';
            }
            else
            {
                $icon = 'ruler-triangle.png';
            }
        }
        elseif ($depth_level == 3)
        {
            if ($key == 'rule')
            {
                $icon = 'ruler.png';
            }
        }
        elseif ($depth_level == 4)
        {
            $icon = 'ticket-small.png';
        }

        return $icon;
    }


    public static function get_title($key, $item, $lk_name)
    {
        if ($key == 'rule' && isset ($item['@attributes']['id']))
        {
            //<rule id="rule_id">

            $title= "<span>".Ossec_utilities::clean_string($key)." </span> id=\"".Ossec_utilities::clean_string($item['@attributes']['id'])."\"";
        }
        elseif (count($item['@attributes']) > 1 && isset($item[0]))
        {
            //<tag attr="attr_value">...</tag>

            unset($item['@attributes'][$lk_name]);

            $keys  = array_keys($item['@attributes']);
            $title = "<span>".Ossec_utilities::clean_string($key)."</span> ".Ossec_utilities::clean_string($keys[0])."=\"".Ossec_utilities::clean_string($item['@attributes'][$keys[0]])."\"";
        }
        else
        {
            //<tag>node_value</tag>
            $value = '';

            if (is_string($item[0]))
            {
                $item[0] = trim($item[0]);
                $value   = (strlen($item[0]) > 18) ? substr($item[0], 0, 18).'...' : $item[0];
                $value   = Ossec_utilities::clean_string($value);
            }

            if (!empty($value))
            {
                $title = "<span>".Ossec_utilities::clean_string($key)."</span> = ".$value;
            }
            else
            {
                $title = "<span>".Ossec_utilities::clean_string($key)."</span>";
            }
        }

        return $title;
    }
}

/* End of file ossec.inc */
/* Location: ../include/classes/ossec.inc */
