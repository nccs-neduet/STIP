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

//Export all hosts
if (isset($_GET['get_data']))
{
    //Setting up a high time limit.
    
	$db   = new ossim_db();
	$conn = $db->connect();
	
	//Setting up the file name with the hosts info
	$file = uniqid('/tmp/export_all_host_' . date('Ymd_H-i-s') . '_');
	$_SESSION['_csv_file_hosts'] = $file;
	
	session_write_close();
        
	$csv = array();
	
	
	//Export a filtered list
	
	$filters = array();
	$session = session_id();
	
    $tables  = ', user_component_filter hc';
    $filters = array(
        'where'    => "hc.asset_id=host.id AND hc.asset_type='asset' AND hc.session_id = '$session'",
        'order_by' => 'host.hostname ASC'
    );
        
	$_host_list = Asset_host::get_list($conn, $tables, $filters);

	foreach($_host_list[0] as $host) 
	{
		$id = $host['id'];
		
		//Description
		
		$descr = $host['descr'];
		$descr = mb_convert_encoding($descr, 'UTF-8', 'HTML-ENTITIES');
		
		//Operating System
		$os = Asset_host_properties::get_property_from_db($conn, $host['id'], 3);
		$os = array_pop($os);
         
        //Latitude/Longitude      
        $latitude   = (empty($host['location']['lat'])) ?  '' : $host['location']['lat'];
        $longitude  = (empty($host['location']['lon'])) ?  '' : $host['location']['lon'];
        
        //Devices
        $str_devices = '';
        
        $devices = Asset_host_devices::get_devices_to_string($conn, $id);
        if (!empty($devices))
        {
            $str_devices = str_replace('<br/>', ',', $devices); 
        }        
                      
		
        $h_data = array();
        $h_data['ips']          = $host['ips'];
        $h_data['name']         = $host['name'];
        $h_data['fqdns']        = $host['fqdns'];
        $h_data['descr']        = $descr;
        $h_data['asset_value']  = $host['asset_value'];
        $h_data['os']           = $os;
        $h_data['latitude']     = $latitude;
        $h_data['longitude']    = $longitude;
        $h_data['id']           = $id;
        $h_data['external']     = $host['external'];
        $h_data['devices']      = $str_devices;        
        
        $csv[] = '"'.implode('";"', $h_data).'"';
	}
	
	$csv_data = implode("\r\n", $csv); 
	file_put_contents($file, $csv_data);
	
	exit();
	
}
elseif (isset($_GET['download_data']))	
{	
	$output_name = _('Assets') .'__' . gmdate('Y-m-d', time()) . '.csv';
	
	$file = $_SESSION['_csv_file_hosts'];
	unset($_SESSION['_csv_file_hosts']);
	
	$csv_data = '"IPs";"Hostname";"FQDNs";"Description";"Asset Value";"Operating System";"Latitude";"Longitude";"Host ID";"External Asset";"Device Type"'."\r\n";
	
	if (file_exists($file))
	{
    	$_csv_data = file_get_contents($file);
    	unlink($file);
	}
	
	$csv_data .= $_csv_data;
	
	
	if (!empty($csv_data))
	{
		header("Expires: Sat, 01 Jan 2000 00:00:00 GMT");
		header("Last-Modified: " . gmdate("D, d M Y H:i:s") . " GMT");
		header("Pragma: public");
		header("Expires: 0");
		header("Cache-Control: must-revalidate, post-check=0, pre-check=0");
		header("Cache-Control: public");
		header("Content-Disposition: attachment; filename=\"$output_name\";");
		header('Content-Type: application/force-download');
		header("Content-Transfer-Encoding: binary");
		header("Content-length: " . strlen($csv_data));
		
		echo $csv_data;
	}
	
	exit();
}

?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
    <meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1"/>
    <title><?php echo _('Export all hosts to CSV')?></title>
    
    <?php 
    //CSS Files
    $_files = array(
        array('src' => 'av_common.css',     'def_path' => TRUE)
    );
    
    Util::print_include_files($_files, 'css');
    
    //JS Files
    $_files = array(
        array('src' => 'jquery.min.js',     'def_path' => TRUE)
    );
    
    Util::print_include_files($_files, 'js');
    ?>
    
    
	<style type="text/css">
		a 
		{
    		cursor:pointer; 
    		font-weight: bold;
		}
		
		#container 
		{ 
			width: 90%;
			margin:auto;
			text-align:center;
			position: relative;
			height: 600px;
		}
		
		#loading 
		{
			position: absolute; 
			width: 99%; 
			height: 99%; 
			margin: auto; 
			text-align: center;
			background: transparent;
			z-index: 10000;
		}
		
		#loading div
		{
			position: relative;
			top: 40%;
			margin:auto;
		}
		
		#loading div span
		{
			margin-left: 5px;
			font-weight: normal;
			font-size: 13px;	
		}
		
		
	</style>
	
	<script type='text/javascript'>
		
		var __cfg = <?php echo Asset::get_path_url() ?>;
		
		function get_csv_data()
		{
    		var url = __cfg.asset.views + "export_all_assets.php";
    		
			$.ajax(
			{
				type: "GET",
				url: url,
				data: "get_data=1",
				success: function(html)
				{			
					$("#export_csv").attr("action", url);
					$("#export_csv").append("<input type='hidden' name='download_data' id='download_data' value='1'/>");
                 	$("#export_csv").attr("target","downloads");
					$("#export_csv").submit();
					
					setTimeout(function()
					{
    					document.location.href = __cfg.asset.views + 'list.php';
    					
    				}, 1000);
				}
			});
		}
						
		$(document).ready(function() 
		{
			setTimeout('get_csv_data()', 2000);	
		});
		
	</script>
	
</head>

<body>
	<div id='container'>
		<div id='loading'>
			<div>
				<img src='/ossim/pixmaps/loading3.gif' />
				<span><?php echo _('Exporting Assets to CSV')?>.</span>
				<span><?php echo _('Please, wait a few seconds')?>...</span>
			</div>
		</div>
	</div>
	
	<form name='export_csv' id='export_csv' action='' method='GET'></form>
	<iframe name='downloads' style='display:none'></iframe>
</body>

</html>
