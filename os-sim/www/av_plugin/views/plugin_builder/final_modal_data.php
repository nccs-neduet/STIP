<?php
require_once 'av_init.php';


if (!Session::am_i_admin()) {
    $config_nt = array(
        'content' => _("You do not have permission to see this section"),
        'options' => array(
            'type'          => 'nf_error',
            'cancel_button' => false
        ),
        'style'   => 'width: 60%; margin: 30px auto; text-align:center;'
    );

    $nt = new Notification('nt_1', $config_nt);
    $nt->show();

    die();
}

?>
<html>

<head>
<link rel="stylesheet" href="css/accordion_css.css">

</head>

<body>
    <table class="final_plgn_tbl">
        <thead>
            <tr>
                <td colspan="2">
                    <h5>Review Plugin Info</h5>
                </td>
            </tr>
        </thead>
        <tbody>
            <tr>
                <td>
                    <div>Plugin File</div>
                </td>
                <td>
                    <div id="plugiName"></div>
                </td>
            </tr>
            <tr>
                <td>
                <div>Vendor</div>
                </td>
                <td>
                <div id="pluginVendor"></div>
                </td>
            </tr>
            <tr>
                <td>
                    <div>Model</div>
                </td>
                <td>
                    <div id="pluginModel"></div>
                </td>
            </tr>
            <tr>
                <td>
                    <div>Version</div>
                </td>
                <td>
                    <div id="pluginVersion"></div>
                </td>
            </tr>
            <tr>
                <td>
                    <div>Product Type</div>
                </td>
                <td>
                    <div id="pluginProductType"></div>
                </td>
            </tr>
            <tr>
                <td>
                    <div>Number of Event Types</div>
                </td>
                <td>
                    <div id="pluginEventCount"></div>
                </td>
            </tr>
        </tbody>
    </table>
    <div style='display:none;'>
        <div><input type="checkbox" id="cbPluginEnable" class="form-control-sm" /></div>
        <div>Enable Plugin</div>
    </div>
    <div>
        <button type="button" class="btn btn-secondary" onclick="close_modal()">Close</button>
        <button type="button" class="btn btn-primary" id="btnPluginFinish">Finish</button>
    </div>
 
</body>
<?php

$_files = array(
    array('src' => 'jquery.min.js',                                         'def_path' => TRUE),
    array('src' => '/av_plugin/views/plugin_builder/js/accordion_js.js',    'def_path' => FALSE),
);
Util::print_include_files($_files, 'js');

?>
<script>
    var body_h = document.body.scrollHeight;
    var html_h = document.documentElement.scrollHeight;

    if (body_h == html_h) {
        var new_h = html_h - 10;
        document.body.style.height = new_h + 'px';
    }


    /*GETTION SESSION STORAGE VALUES*/
    var byId = function(id) {
        return document.getElementById(id);
    };

    var arr = JSON.parse(sessionStorage.getItem("plugin_details"));
    for (i = 0; i < arr.length; i++) {

        byId(arr[i][0]).innerHTML = arr[i][1];

    }
</script>

</html>