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
// THIS SCRIPT IS DEPRECATED, NOW USING /conf/reload.php?what=directives TO RESTART SERVER
require_once 'av_init.php';

Session::logcheck("configuration-menu", "CorrelationDirectives");


$login = Session::get_session_user();
$db    = new ossim_db();
$conn  = $db->connect();

$action = POST('action');
$data   = POST('data');

ossim_valid($action,	OSS_DIGIT,	'illegal:' . _('Action'));
if (ossim_error())
{
    die(ossim_error());
}

if($action != '' && isset($_SERVER['HTTP_X_REQUESTED_WITH']) && strtolower($_SERVER['HTTP_X_REQUESTED_WITH']) == 'xmlhttprequest'){

	switch($action){
	
		case 1:			
			$response = Directive_editor::restart_directives();			
			break;
							
		default:
			$response['error'] = true ;
			$response['msg']   = 'Wrong Option Chosen';
	}
	
	echo json_encode($response);
}

$db->close($conn);
?>
