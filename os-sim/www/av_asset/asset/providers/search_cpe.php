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


require_once 'av_init.php';

Session::logcheck('environment-menu', 'PolicyHosts');


//CPE Types
$_cpe_types = array(
    'os'       => 'o',
    'hardware' => 'h',
    'software' => 'a'
);

$_cpe      = GET('q');
$_cpe_type = GET('cpe_type');

	
ossim_valid($_cpe, OSS_NULLABLE, OSS_ALPHA, OSS_PUNC_EXT, 'illegal:' . _('CPE'));
ossim_valid($_cpe_type, 'os | software | hardware',       'illegal:' . _('CPE Type'));
	
if (ossim_error() || !array_key_exists($_cpe_type, $_cpe_types))
{ 
	exit();
}


$db     = new Ossim_db();
$conn   = $db->connect();

$_cpe   = escape_sql($_cpe, $conn);

$filters = array(
    'where' => "`cpe` LIKE 'cpe:/".$_cpe_types[$_cpe_type]."%' AND `line` LIKE '%$_cpe%'",
    'limit' => 20
);

$software = new Software($conn, $filters);

$db->close();

foreach ($software->get_software() as $cpe_info) 
{
    echo  $cpe_info['cpe'].'###'.$cpe_info['line']."\n";
}

/* End of file search_cpe.php */
