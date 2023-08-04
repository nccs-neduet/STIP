<?php
/*******************************************************************************
** OSSIM Forensics Console
** Copyright (C) 2009 OSSIM/AlienVault
** Copyright (C) 2004 BASE Project Team
** Copyright (C) 2000 Carnegie Mellon University
**
** (see the file 'base_main.php' for license details)
**
** Built upon work by Roman Danyliw <rdd@cert.org>, <roman@danyliw.com>
** Built upon work by the BASE Project Team <kjohnson@secureideas.net>
*/

header("Expires: Mon, 26 Jul 1997 05:00:00 GMT");
header("Last-Modified: " . gmdate("D, d M Y H:i:s") . "GMT");
header("Cache-Control: no-cache, must-revalidate");
header("Pragma: no-cache");

require_once 'av_init.php';
Session::logcheck("analysis-menu", "EventsForensics");

list($ip,$ctx,$net_id) = explode(";",GET('ip'));

ossim_valid($ip, OSS_IP_ADDR_0,             'illegal:' . _('IP address'));
ossim_valid($ctx, OSS_HEX,                  'illegal:' . _('CTX'));
ossim_valid($net_id, OSS_HEX, OSS_NULLABLE, 'illegal:' . _('Net Id'));

if (ossim_error())
{
    die(ossim_error());
}


$db = new ossim_db(TRUE);

if (is_array($_SESSION['server']) && $_SESSION['server'][0] != '')
{
    $conn = $db->custom_connect($_SESSION["server"][0],$_SESSION["server"][2],$_SESSION["server"][3]);
}
else
{
    $conn = $db->connect();
}

$net = NULL;

if ($net_id != '')
{
    if ($net_obj = Asset_net::get_object($conn, $net_id))
    {
        $net = array(
            'name'  => $net_obj->get_name(),
            'ips'   => $net_obj->get_ips(),
            'icon'  => $net_obj->get_html_icon()
        );
    }
}
else
{
    $net = array_shift(Asset_host::get_closest_net($conn, $ip, $ctx));

    if ($net['icon'] != '')
    {
        $net['icon'] = "<img class='asset_icon w16' src='data:image/png;base64,".base64_encode($net['icon'])."'/>";
    }
}


if (is_array($net) && !empty($net))
{
    echo $net['icon']."<strong>".$net['name']."</strong> (".$net['ips'].")";
}
else
{
    echo "<strong>$ip</strong> "._("not found in home networks");
}

$db->close();
