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


if (!Session::am_i_admin())
{
    $config_nt = array(
            'content' => _("You do not have permission to see this section"),
            'options' => array (
                    'type'          => 'nf_error',
                    'cancel_button' => false
            ),
            'style'   => 'width: 60%; margin: 30px auto; text-align:center;'
    );
    
    $nt = new Notification('nt_1', $config_nt);
    $nt->show();

    die();
}

$tz = Util::get_timezone();
?>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
    <title><?php echo _('AlienVault ' . (Session::is_pro() ? 'USM' : 'OSSIM')) ?></title>
    <meta http-equiv="Content-Type" content="text/html;charset=iso-8859-1"/>
    <meta http-equiv="Pragma" content="no-cache"/>

    <?php

        //CSS Files
        $_files = array(
            array('src' => 'av_common.css',                                            'def_path' => TRUE),
            array('src' => 'jquery-ui.css',                                            'def_path' => TRUE),
            array('src' => 'tipTip.css',                                               'def_path' => TRUE),
            array('src' => 'jquery.dataTables.css',                                    'def_path' => TRUE),
            array('src' => 'jquery.dropdown.css',                                      'def_path' => TRUE),
            array('src' => 'av_table.css',                                             'def_path' => TRUE),
        );
        
        Util::print_include_files($_files, 'css');
    echo '<style>';
    echo Util::get_css_content('/usr/share/ossim/www/av_plugin/views/plugin_builder/css/accordion_css.css')."\n";
    echo '</style>';
        //JS Files
        $_files = array(
            array('src' => 'jquery.min.js',                                 'def_path' => TRUE),
            array('src' => 'jquery-ui.min.js',                              'def_path' => TRUE),
            array('src' => 'jquery.number.js.php',                          'def_path' => TRUE),
            array('src' => 'utils.js',                                      'def_path' => TRUE),
            array('src' => 'notification.js',                               'def_path' => TRUE),
            array('src' => 'token.js',                                      'def_path' => TRUE),
            array('src' => 'jquery.tipTip.js',                              'def_path' => TRUE),
            array('src' => 'greybox.js',                                    'def_path' => TRUE),
            array('src' => 'jquery.dataTables.js',                          'def_path' => TRUE),
            array('src' => 'av_table.js.php',                               'def_path' => TRUE),
            array('src' => 'av_storage.js.php',                             'def_path' => TRUE),
            array('src' => 'jquery.md5.js',                                 'def_path' => TRUE),
            array('src' => 'jquery.placeholder.js',                         'def_path' => TRUE),
            array('src' => 'jquery.dropdown.js',                            'def_path' => TRUE),
            array('src' => '/av_plugin/views/plugin_builder/js/accordion_js.js',    'def_path' => FALSE),
            array('src' => '/av_plugin/views/plugin_builder/js/regex-generator.js',    'def_path' => FALSE),
        );
        
        Util::print_include_files($_files, 'js');

    ?>
    
    <script type='text/javascript'>

        /**********  LIGHTBOX EVENTS  **********/
        function GB_onclose(url, params) {
            if (url != "stip/av_plugin/views/add_plugin.php") return;
            $.post("stip/session/token.php", {
                'f_name': 'plugin_actions'
            }, function(data) {
                data = {
                    "token": $.parseJSON(data).data,
                    "action": "rollback"
                }
                $.post('<?php echo AV_MAIN_PATH . "/av_plugin/controllers/plugin_actions.php" ?>', data);
			location.href = location.href;
		});
	}

        function GB_onhide(url, params)
        {
            if (typeof params == 'undefined')
            {
                params = {};
            }

            // Linked from wizard finish 'Configure Event Types' button
            if (typeof params['plugin_id'] != 'undefined' && params['plugin_id'] != '')
            {
                var _sids_url          = '<?php echo AV_MAIN_PATH . '/conf/pluginsid.php?plugin_id=' ?>' + params['plugin_id'];
                var _url               = top.av_menu.get_menu_url(_sids_url, 'configuration', 'threat_intelligence', 'data_source');
                document.location.href = _url;
            }
        }    
    
        $(document).ready(function() 
        {
            // Error from the API
            <?php if ($error != '')
            {
            ?>
            show_notification('plugin_notif', "<?php echo $error ?>", 'nf_error');
            <?php
            }
            ?>
            

            // ToolTips
            $(".info").tipTip({maxWidth: '380px'});

            // Go back to Plugin Page
            $('[data-bind="list-plugin"]').click(function() {
                document.location.assign("/ossim/av_plugin/index.php?m_opt=configuration&sm_opt=deployment&h_opt=plugins");
            });

        });

    </script>
    

</head>

<body>

<?php 
    //Local menu
    include_once '../local_menu.php';
?>
    
    <div id='main_container'>
        
        <div class="content">
            
            <div id="plugin_notif"></div>

            <div id='plugin_section_title'>
                <?php echo _('Plugins') ?>
            </div>
            
            <div id='content_result'>
                                
                <div id='action_buttons'>
                    
                    <input type='button' id='back_button' data-bind='list-plugin' value='<?php echo _('Go Back') ?>' />
    
                </div>
                <!-- Including accordion -->
                <?php
                    include_once 'views/plugin_builder/accordion_data.php';
                ?>
                </div>
                

            </div>
            
            
        </div>
        
    </div>

</body>

</html>


