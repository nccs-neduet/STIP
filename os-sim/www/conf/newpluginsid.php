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

Session::logcheck('configuration-menu', 'ConfigurationPlugins');

$validate = array(
	'plugin_id'     => array('validation'=>'OSS_DIGIT',                                      'e_message' => 'illegal:' . _('Plugin')),
	'ds_name'   	=> array('validation'=>'OSS_SCORE, OSS_ALPHA, OSS_SPACE, OSS_PUNC_EXT',  'e_message' => 'illegal:' . _('Name')),
	'sid'         	=> array('validation'=>'OSS_DIGIT',                                      'e_message' => 'illegal:' . _('SID')),
	'reliability'   => array('validation'=>'OSS_DIGIT',                              	     'e_message' => 'illegal:' . _('Reliability')),
	'priority'    	=> array('validation'=>'OSS_DIGIT',                              	     'e_message' => 'illegal:' . _('Priority')),
	'category'      => array('validation'=>'OSS_DIGIT, OSS_NULLABLE',                        'e_message' => 'illegal:' . _('Category')),
	'subcategory'   => array('validation'=>'OSS_DIGIT, OSS_NULLABLE',                        'e_message' => 'illegal:' . _('Subcategory'))
	
);

if (GET('ajax_validation') == TRUE)
{
	$data['status'] = 'OK';
	
	$validation_errors = validate_form_fields('GET', $validate);
	if ( is_array($validation_errors) && !empty($validation_errors) )	
	{
		$data['status'] = 'error';
		$data['data']   = $validation_errors;
	}
	
	echo json_encode($data);	
	exit();
}


$db   = new ossim_db();
$conn = $db->connect();

$plugin_id   = POST('plugin_id');
$name        = POST('ds_name');
$sid         = POST('sid');
$reliability = POST('reliability');
$priority    = POST('priority');
$category    = POST('category');
$subcategory = POST('subcategory');
	
$validation_errors = validate_form_fields('POST', $validate);
	
$data['status'] = 'OK';

$list_categories = Category::get_list($conn);

if ($sid != '' && $plugin_id != '' && $validation_errors['plugin_id'] == '')
{
	if (in_array($sid, Plugin_sid::get_sids_by_id($conn, $plugin_id)))
	{
		$validation_errors['plugin_id'] = _("Event type $sid already exists");
	}
	
	if ($sid < 1)
	{
		$validation_errors['sid'] = _('Sid must be a valid number higher than 0');
	}
}

if ($priority < 0 || $priority > 5)
{
	$validation_errors['priority'] = _('Priority must be between 0 and 5');
}

if ($reliability < 0 || $reliability > 10)
{
	$validation_errors['reliability'] = _('reliability must be between 0 and 10');
}

$db->close();

$data['data'] = $validation_errors;
	
if (POST('ajax_validation_all') == TRUE)
{
	if (is_array($validation_errors) && !empty($validation_errors))
	{
		$data['status'] = 'error';
		echo json_encode($data);
	}
	else
	{
		$data['status'] = 'OK';
		echo json_encode($data);
	}
	
	exit();
}
else
{
	if (is_array($validation_errors) && !empty($validation_errors))
	{
		$data['status'] = 'error';
	}
}


?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
	<title><?php echo gettext('OSSIM Framework');?></title>
	<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1"/>
	<meta http-equiv="Pragma" content="no-cache">
	<link type="text/css" rel="stylesheet" href="../style/av_common.css?t=<?php echo Util::get_css_id() ?>"/>
</head>

<body>

<?php
if ($data['status'] == 'error')
{
	$txt_error = '<div>'._('The following errors occurred').":</div>
				  <div style='padding:2px 10px 5px 10px;'>".implode('<br/>', $validation_errors).'</div>';				
			
	$config_nt = array(
		'content' => $txt_error,
		'options' => array (
			'type'          => 'nf_error',
			'cancel_button' => FALSE
		),
		'style'   => 'width: 80%; margin: 20px auto; text-align: left;'
	); 
					
	$nt = new Notification('nt_1', $config_nt);
	$nt->show();

	Util::make_form('POST', 'plugin.php');
	exit();
}

if ($category == '' || $subcategory == '')
{
	$category    = '';
	$subcategory = '';
}

$db   = new ossim_db();
$conn = $db->connect();

Plugin_sid::insert($conn, $plugin_id, $name, $sid, $reliability, $priority, $category, $subcategory);
Util::resend_asset_dump();

$db->close();

?>
<script type='text/javascript'>
	document.location.href="pluginsid.php?plugin_id=<?php echo $plugin_id?>&msg=created";
</script>

</body>
</html>
