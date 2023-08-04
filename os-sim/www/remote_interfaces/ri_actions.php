<?php
/**
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
*/


//Config File
require_once 'av_init.php';

Session::logcheck('configuration-menu', 'PolicyServers');

session_write_close();

//Validate action type

$action  = POST('action');

ossim_valid($action, OSS_LETTER, '_', 'illegal:' . _('Action'));

if (ossim_error()) 
{
    $data['status']  = 'error';
	$data['data']    = ossim_get_error_clean();
	
	echo json_encode($data);
    exit();
}


//Validate Form token

$token = POST('token');

if (Token::verify('tk_ri_form', POST('token')) == FALSE)
{		
	$data['status']  = 'error';
	$data['data']    = Token::create_error_message();
	
	echo json_encode($data);
    exit();	
}

switch($action)
{    
    case 'delete_ri':
        
        $validate = array(
            'id'  =>  array('validation' => 'OSS_DIGIT',  'e_message'  =>  'illegal:' . _('Remote Interface ID'))
        );   
                
        $id = POST('id');
        
        $validation_errors = validate_form_fields('POST', $validate);
            
            
        if (is_array($validation_errors) && !empty($validation_errors))
        {
            $data['status'] = 'error';
            $data['data']   = _('Error! Remote Interface ID not allowed.  Remote interface could not be removed');
        }
        else
        {
            try
            {
                $db    = new ossim_db();
                $conn  = $db->connect();
            
                Remote_interface::delete($conn, $id);                
               
                $aux_ri_interfaces = Remote_interface::get_list($conn, "WHERE status = 1");                  
                
                $db->close();
                
                $data['status']         = 'OK';                
                $data['data']           = _('Remote Interface removed successfully');
                $data['num_interfaces'] = $aux_ri_interfaces[1];      
            }
            catch(Exception $e)
            {
                $data['status'] = 'error';
                $data['data']   = _('Error! Remote Interface could not be removed');
            }
        }
         
    break;
}

echo json_encode($data);
exit();
?>
