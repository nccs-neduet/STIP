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


require_once dirname(__FILE__).'/../../conf/config.inc';;

Session::logcheck('environment-menu', 'EventsHidsConfig');


$title = ($editable == TRUE) ? _("Edit node:  $node_name ") : _("Show node:  $node_name ");

?>

<div id='edit_container'>

    <table id='header_rule'>
        <tbody>
            <tr><td class='sec_title'><?php echo $title?></td></tr>
        </tbody>
    </table>

    <form name='form_m' id='form_m'>

        <table class='er_container' id='erc1'>
            <tbody id='erb_c1'>
                <?php
                echo Ossec_utilities::print_subheader('attributes', $editable);

                $at_data = array(
                    'data'        => $attributes,
                    'img_path'    => '/ossim/ossec/images',
                    'is_editable' => $editable,
                    'lk_name'     => $lk_name
                );

                echo Ossec_utilities::print_attributes($at_data);
                ?>
            </tbody>
        </table>

        <?php echo Ossec_utilities::print_subfooter($sf_data, $editable);?>
    </form>
</div>
