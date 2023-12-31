<?php
/**
* provider.inc
*
* File provider.inc is used to:
*   - manage the AlienVault REST API providers
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
* @package    ossim-framework\Various
* @autor      AlienVault INC
* @license    http://www.gnu.org/licenses/gpl-2.0.txt
* @copyright  2003-2006 ossim.net
* @copyright  2007-2013 AlienVault
* @link       https://www.alienvault.com/
*/


/**
* The goal of this class is to define the providers common functions and attributes
*
*
* @package    ossim-framework\Various
* @autor      AlienVault INC
* @license    http://www.gnu.org/licenses/gpl-2.0.txt
* @copyright  2003-2006 ossim.net
* @copyright  2007-2013 AlienVault
* @link       https://www.alienvault.com/
*/
class Provider
{
    /**
    * Connection object
    * @access protected
    * @var object
    *
    */
    protected $conn;

    /**
    * Providers objects
    * @access protected
    * @var object
    *
    */
    protected $sub_providers;

    /**
    * common url to do the request
    * @access private
    * @var string
    *
    */
    protected $common_url;

    /**
    * The class constructor
    *
    * This sets up the providers classes
    *
    * @param object  $conn        Database access object
    * @param string  $common_url  Common url to do the request
    *
    */
    public function __construct($conn, $common_url)
    {
        $this->conn          = $conn;

        $this->sub_providers = new Provider_registry();

        $this->common_url    = $common_url;
    }


    /**
    * This function returns the URL related to the provider
    *
    * @return string
    */
    public function get_common_url()
    {
        return $this->common_url;
    }


    /**
    * This function sets a new URL for the provider
    *
    * @param string  $common_url  Provider URL
    *
    * @return void
    */
    public function set_common_url($common_url)
    {
        $this->common_url = $common_url;
    }
}

/**
* The goal of this class is to registry the AlienVault API providers
*
* <br/>
* <pre>
* <code>
* Example usage:
*
* $providers = new Provider_registry();
*
* </code>
* </pre>
*
* @package    ossim-framework\Various
* @autor      AlienVault INC
* @license    http://www.gnu.org/licenses/gpl-2.0.txt
* @copyright  2003-2006 ossim.net
* @copyright  2007-2013 AlienVault
* @link       https://www.alienvault.com/
*/
class Provider_registry
{
    /**
    * Registry of variables and objects
    * @access private
    * @var array
    */
    private $registry;

    /**
    * The class constructor
    *
    * This sets up the class
    *
    */
    public function __construct()
    {
        $this->registry = array();
    }

    /**
     * The class factory
     *
     * This factory the class
     *
     */
    public static function factory()
    {
        return new Provider_registry();
    }

    /**
    * Sets the current provider if the provider is not registered, we register it
    *
    * @access public
    * @param string item's unique name
    * @param mixed item
    *
    * @return boolean
    */

    /**
     * Sets the current provider if the provider is not registered, we register it
     *
     * @param object  $conn    Database access object
     * @param string  $name    Provider name
     * @param string  $section Section to go
     * @return bool
     * @throws Exception  If a connection error occurred
     */
    public function set($conn, $name, $section)
    {
        $result = FALSE;

        if (!$this->exists($name))
        {
            $object = Provider_factory::Create($conn, $name, $section);

            if($result!==NULL)
            {
                $this->registry[$name] = $object;

                $result = TRUE;
            }
        }
        else
        {
            //URL could have changed, we set up it again
            $this->registry[$name]->set_common_url($section);
        }

        return $result;
    }

    /**
     * Returns true if item was registered
     *
     * @param string  $name Provider name
     * @return bool
     * @throws Exception
     */
    public function exists($name)
    {
        if (is_string($name))
        {
            return array_key_exists($name, $this->registry);
        }
        else
        {
            throw new Exception('Registry item\'s name must be a string');
        }
    }


    /**
    * Returns registered item
    *
    * @access public
    * @param string item's name
    *
    * @return mixed (null if name is not in registry)
    */
    public function get($name)
    {
        if (array_key_exists($name, $this->registry))
        {
            $return = $this->registry[$name];
        }
        else
        {
            $return = NULL;
        }

        return $return;
    }


    /**
    * Removes a registry entry
    *
    * @access public
    * @param string item's name
    *
    * @return boolean
    */
    public function remove($name)
    {
        if (array_key_exists($name, $this->registry))
        {
            unset($this->registry[$name]);
        }

        return TRUE;
    }


    /**
    * Clears the entire registry
    *
    * @access public
    *
    * @return boolean
    */
    public function clear()
    {
        $this->registry = array();
    }
}

/**
* The goal of this class is to create the AlienVault API providers
*
* <br/>
* <pre>
* <code>
* Example usage:
*
* $object = Provider_factory::Create($conn, 'Config', '/av/api/1.0')
*
*
* </code>
* </pre>
*
* @package    ossim-framework\Various
* @autor      AlienVault INC
* @license    http://www.gnu.org/licenses/gpl-2.0.txt
* @copyright  2003-2006 ossim.net
* @copyright  2007-2013 AlienVault
* @link       https://www.alienvault.com/
*/

class Provider_factory
{
    /**
     * Returns the created provider object
     *
     * @access static  public
     * @param  object  $conn        Connection object
     * @param  string  $name        Provider name
     * @param  string  $common_url  Common url to do the request
     *
     * @return object
     */
    public static function Create($conn, $name, $common_url)
    {
        switch ($name)
        {
            // Auth
            case 'auth':
                require_once __DIR__ . '/auth.inc';

                $result = new Auth_provider($conn, $common_url);

            break;

            // Sensor
            case 'sensor':
                require_once __DIR__ . '/sensor.inc';

                $result = new Sensor_provider($conn, $common_url);

            break;

            // NMAP
            case 'nmap':
                require_once __DIR__ . '/nmap.inc';

                $result = new Nmap_provider($conn, $common_url);
            break;

            // GVM
            case 'gvm':
                require_once __DIR__ . '/gvm.inc';

                $result = new Gvm_provider($conn, $common_url);

            break;

            // System
            case 'system':
                require_once __DIR__ . '/system.inc';

                $result = new System_provider($conn, $common_url);

            break;

            // Server
            case 'server':
                require_once __DIR__ . '/server.inc';

                $result = new Server_provider($conn, $common_url);

            break;

            // Data
            case 'data':
                require_once __DIR__ . '/data.inc';

                $result = new Data_provider($conn, $common_url);

            break;

            // Jobs
            case 'jobs':
                require_once __DIR__ . '/jobs.inc';

                $result = new Jobs_provider($conn, $common_url);

            break;


            // ASEC
            case 'asec':
                require_once __DIR__ . '/asec.inc';

                $result = new Asec_provider($conn, $common_url);

            break;


            // Plugin
            case 'plugin':
                require_once __DIR__ . '/plugin.inc';

                $result = new Plugin_provider($conn, $common_url);

            break;

            case 'central_console':
                require_once __DIR__ . '/central_console.inc';
                $result = new CentralConsoleProvider($conn, $common_url);
            break;



            default:

                $result = NULL;
        }

        return $result;
    }
}

/* End of file provider.inc */
/* Location: ../include/classes/providers/provider.inc */
