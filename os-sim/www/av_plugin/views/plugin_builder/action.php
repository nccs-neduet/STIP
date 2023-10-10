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

$db    = new ossim_db();
$conn  = $db->connect();

$action = $_REQUEST['action'];

if ($action == 'logUpload') {
    if (isset($_FILES['file'])) {
        $errors = array();
        $file_name = $_FILES['file']['name'];
        $file_size = $_FILES['file']['size'];
        $file_tmp = $_FILES['file']['tmp_name'];
        $file_type = $_FILES['file']['type'];
        $a = explode('.', $file_name);
        $b = end($a);
        $file_ext = strtolower($b);

        $extensions = array("log", "txt");

        if (in_array($file_ext, $extensions) === false) {
            $response['status'] = "ERROR";
            $errors[] = "extension not allowed, please choose a JPEG or PNG file.";
            $response["data"] = "extension not allowed, please choose a JPEG or PNG file.";
        }

        if ($file_size > 2097152) {
            $response['status'] = "ERROR";
            $errors[] = 'File size must be excately 2 MB';
            $response["data"] = "File size must be excately 2 MB";
        }

        if (empty($errors) == true) {

            $data = file($file_tmp, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES); //read the entire file to array by ignoring new lines and spaces

            $newData = array();

            foreach ($data as $value) {
                $temp = str_split($value);
                $newStr = array();
                for ($i = 0; $i < count($temp); $i++) {
                    if ($temp[$i] == ' ' && $temp[$i + 1] != ' ') {
                        $newStr[] = $temp[$i] . ' ';
                    } else if ($temp[$i] == '(' ||  $temp[$i] == '[') {
                        if ($temp[$i + 1] != ' ' && $temp[$i - 1] != ' ') {
                            if($temp[$i + 1] == '/')
                            {
                                $newStr[] = ' ' . $temp[$i];
                            }
                            else{
                                $newStr[] = ' ' . $temp[$i] . ' ';
                            }
                        } 
                        else if ($temp[$i + 1] != ' ' && $temp[$i - 1] == ' ') {
                            $newStr[] = $temp[$i] . ' ';
                        }
                        else if ($temp[$i + 1] == ' ' && $temp[$i - 1] != ' ') {
                            $newStr[] = ' ' . $temp[$i];
                        }
                    } else if ($temp[$i] == ',') {
                        if ($temp[$i + 1] != ' ') {
                            $newStr[] = ' ' . $temp[$i] . ' ';
                        } else {
                            $newStr[] = ' ' . $temp[$i];
                        }
                    } 
                    else if ($temp[$i] == '/') {
                        if ($temp[$i + 1] != ' ' && $temp[$i - 1] != ' ') {
                            $newStr[] = ' ' . $temp[$i] . ' ';
                        } 
                        else if ($temp[$i + 1] != ' ' && $temp[$i - 1] == ' ') {
                            $newStr[] = $temp[$i] . ' ';
                        } 
                        else {
                            $newStr[] = ' ' . $temp[$i];
                        }
                    } else if ($temp[$i] == '-') {
                        if ($temp[$i + 1] != ' ' && $temp[$i - 1] != ' ') {
                            $newStr[] = ' ' . $temp[$i] . ' ';
                        } 
                        else if ($temp[$i + 1] != ' ' && $temp[$i - 1] == ' ') {
                            $newStr[] = $temp[$i] . ' ';
                        } 
                        else {
                            $newStr[] = ' ' . $temp[$i];
                        }
                    } else if ($temp[$i] == '%') {
                        if ($temp[$i + 1] != ' ' && $temp[$i - 1] != ' ') {
                            $newStr[] = ' ' . $temp[$i] . ' ';
                        } 
                        else if ($temp[$i + 1] != ' ' && $temp[$i - 1] == ' ') {
                            $newStr[] = $temp[$i] . ' ';
                        } 
                        else {
                            $newStr[] = ' ' . $temp[$i];
                        }
                    } else if ($temp[$i] == '=') {
                        if ($temp[$i + 1] != ' ') {
                            $newStr[] = ' ' . $temp[$i] . ' ';
                        } else {
                            $newStr[] = ' ' . $temp[$i];
                        }
                    } else if ($temp[$i] == ')' || $temp[$i] == ']') {
                        $newStr[] = ' ' . $temp[$i];
                    } else if ($temp[$i] == ';') {
                        if ($temp[$i - 1] != ' ') {
                            $newStr[] = ' ' . $temp[$i];
                        } else {
                            $newStr[] = $temp[$i];
                        }
                    } else if ($temp[$i] == ':' && (!is_numeric($temp[$i - 1]) && !is_numeric($temp[$i + 1]))) {
                        if ($temp[$i + 1] != ' ') {
                            if($temp[$i + 1] == '[' || $temp[$i + 1] == '(' || $temp[$i + 1] == '/')
                            {
                                $newStr[] = ' ' . $temp[$i];    
                            }
                            else{
                                $newStr[] = ' ' . $temp[$i] . ' ';
                            }
                        } else if ($temp[$i + 1] == ' ' && $temp[$i - 1] != ' ') {
                            $newStr[] = ' ' . $temp[$i];
                        } else {
                            $newStr[] = $temp[$i];
                        }
                    } else {
                        $newStr[] = $temp[$i];
                    }
                }
                $newData[] = implode($newStr);
            }

            $response["status"] = "OK";
            $response["data"] = $newData;


            echo json_encode($response);
        } else {
            echo json_encode($response);
        }
    }
} else if ($action == "regexGenerated") {
    $regexData = json_decode($_REQUEST['regexs']);
    $tableRow = '';
    for ($i = 0; $i < count($regexData); $i++) {
        $tableRow .= "<tr class='regex-row'><td>" . $regexData[$i][0] . "</td><td style='display:none;'>" . $regexData[$i][1] . "</td><td><button class='toggle_greybox' href='../av_plugin/views/plugin_builder/modal_data.php' onClick='toggle_modal(this)' data-title='My Modal' data-log='" . $regexData[$i][0] . "' data-regex='" . $regexData[$i][1] . "'><svg viewBox='0 0 15 15' fill='none' xmlns='http://www.w3.org/2000/svg' width='19' height='19'><path d='M10.854.146a.5.5 0 00-.708 0L0 10.293V14.5a.5.5 0 00.5.5h4.207L14.854 4.854a.5.5 0 000-.708l-4-4z' fill='currentColor'></path></svg></button> | <button class='btn btn-sm btn-danger deleteModalEntry'><svg viewBox='0 0 15 15' fill='none' xmlns='http://www.w3.org/2000/svg' width='19' height='19'><path fill-rule='evenodd' clip-rule='evenodd' d='M11 3V1.5A1.5 1.5 0 009.5 0h-4A1.5 1.5 0 004 1.5V3H0v1h1v9.5A1.5 1.5 0 002.5 15h10a1.5 1.5 0 001.5-1.5V4h1V3h-4zM5 1.5a.5.5 0 01.5-.5h4a.5.5 0 01.5.5V3H5V1.5zM7 7v5h1V7H7zm-3 5V9h1v3H4zm6-3v3h1V9h-1z' fill='currentColor'></path></svg></button></td><td class='align-middle eventStatus'>Pending</td></tr>";
    }


    $query2    = "select id,name from product_type;";
    $rs = $conn->Execute($query2);
    $opts = '';
    while (!$rs->EOF) {
        $plgn_id = $rs->fields['id'];
        $plgn_name = $rs->fields['name'];
        $opts .= '<option value="' . $plgn_id . '">' . $plgn_name . '</option>';
        $rs->MoveNext();
    }
    mysqli_free_result($rs);


    $query3 = 'select id,name from category;';
    $rs = $conn->Execute($query3);

    $cats = '';
    while (!$rs->EOF) {
        $plgn_id2 = $rs->fields['id'];
        $plgn_cat = $rs->fields['name'];
        $cats .= '<option value="' . $plgn_id2 . '">' . $plgn_cat . '</option>';
        $rs->MoveNext();
    }
    mysqli_free_result($rs);



    $query5 = 'select id,name from subcategory;';
    $rs = $conn->Execute($query5);
    $cls = '';
    while (!$rs->EOF) {
        $plgn_id3 = $rs->fields['id'];
        $plgn_subcat = $rs->fields['name'];
        $cls .= '<option value="' . $plgn_id3 . '">' . $plgn_subcat . '</option>';
        $rs->MoveNext();
    }
    mysqli_free_result($rs);


    $query    = "CALL generate_pluginid();";
    $rs = $conn->Execute($query);
    $p_id = $rs->fields['id'];
    mysqli_free_result($rs);

 
    $htmlResponse = ' <div class="plgn_form_cont">
    <p>Plugin Details</p>
    <div class="col-md-12  card py-3">
        <div class="form-row">
            <div class="col-md-4 mb-3">
                <label for="validationCustom01">Plugin Id*</label>
                <input type="text" class="form-control" id="txtPluginId" value="' . $p_id . '" required readonly disabled>
            </div>
            <div class="col-md-4 mb-3" style="position:relative;">
                <label for="validationCustom01">Enter Plugin Name*</label>
                <input type="text" class="form-control" id="txtPluginName" placeholder="Plugin Name" required onblur="check_plgn_name(this)">
                <span id="availability" style="position:absolute; right:24px; bottom:-14px;"></span>
            </div>
            <div class="col-md-4 mb-3">
                <label for="validationCustom01">Product Type*</label>
                <select name="txtProductType" id="txtProductType" class="form-control">
                    <option value="">Select Product Type</option>
                    ' . $opts . '
                </select>
            </div>
            <div class="col-md-4 mb-3">
                <label for="validationCustom02">Vendor*</label>
                <input type="text" class="form-control" id="txtVendor" placeholder="Vendor" required>
            </div>
            <div class="col-md-4 mb-3">
                <label for="validationCustomUsername">Model*</label>
                <div class="input-group">
                    <input type="text" class="form-control" id="txtModel" placeholder="Model" required>
                </div>
            </div>
            <div class="col-md-4 mb-3">
                <label for="validationCustom03">Version</label>
                <input type="text" class="form-control" id="txtVersion" placeholder="Version">
            </div>
            <div class="col-md-12 mb-3">
                <label for="validationCustom03">Description</label>
                <textarea class="form-control" name="txtDescription" id="txtDescription"></textarea>
            </div>
        </div>
    </div>
</div>

<table class="table_list plugin_table" id="tblEvent">
                <thead>
                    <tr>
                        <th>Log Line</th>
                        <th style="display:none;">Regex</th>
                        <th>Action</th>
                        <th>Status</th>
                    </tr>
                </thead>
                <tbody>
                    ' . $tableRow . '
                </tbody>
            </table>
            <div class="row my-1">
                <div class="final_nxt_btn_parent_div">
                    <button type="button" id="btnFinalNext" href="../av_plugin/views/plugin_builder/final_modal_data.php" data-title="Finalize Plugin Details"  disabled>Next</button>
                </div>
            </div>

            <script>
  
                /** ------------Check Plugin detail fields Start------------**/

                var fields = "#txtPluginName, #txtProductType, #txtVendor, #txtModel";

                $(document).on("change", fields, function() {
                    if (allFilled()) {
                        $("#btnFinalNext").removeAttr("disabled");
                    } else {
                        $("#btnFinalNext").attr("disabled", "disabled");
                    }
                });

                function allFilled() {
                    var filled = true;
                    $(fields).each(function() {
                        if ($(this).val() == "") {
                            filled = false;
                        }
                    });
                    return filled;
                }

                /** ------------Check Plugin detail fields End------------**/

                $(".loader-ovrd").hide();
            </script>';


    $response["status"] = "OK";
    $response["data"] = $htmlResponse;

    echo json_encode($response);
} else if ($action == 'checkplugname') {
    if (isset($_POST['plugin_name'])) {
        $pgname = $_POST['plugin_name'];


        $sql = "SELECT count(*) FROM plugin where name = ?";
        $stmt = $conn->prepare($sql);
        $result = $conn->execute($stmt, $pgname);
        if ($row = $result->fetchRow()) {
            if ($row[0] == 0 and $row[0] != null) {
                $data['status'] = "OK";
                $data["data"] = $row[0];
                echo json_encode($data);
            } else {
                $data['status'] = "ERROR";
                $data["data"] = "Name already exist";
                echo json_encode($data);
            }
        }
    } else {
        $data['status'] = "Error";
        $data["data"] = "If failed";
    }
} else if ($action == 'fetchsubcat') {
    if (isset($_POST['catid'])) {
        $catid = $_POST['catid'];

        $sql = "select id,name from subcategory where cat_id = ?";
        $stmt = $conn->prepare($sql);
        $result = $conn->execute($stmt, $catid);

        $subcats = '';
        while ($row = $result->fetchRow()) {

            $subcats .= '<option value="' . $row['id'] . '">' . $row['name'] . '</option>';
        }

        $data['status'] = "OK";

        $data["data"] = $subcats;
        echo json_encode($data);
    } else {
        $data['status'] = "Error";
        $data["data"] = "If failed";
        echo json_encode($data);
    }
} else if ($action == 'generateplugin') {
    $data = array();
    if (isset($_POST['pgid']) && isset($_POST['pgname']) && isset($_POST['prodType']) && isset($_POST['vendor']) && isset($_POST['model'])) {

        $pgid = $_POST['pgid'];
        $pgname = $_POST['pgname'];
        $prodType = $_POST['prodType'];
        $vendor = $_POST['vendor'];
        $model = $_POST['model'];
        $version = $_POST['version'];
        $descp = $_POST['descp'];
        $eventdata = $_POST['eventdata'];

        $status = generate_plugin($conn, $pgid, $pgname, $prodType, $vendor, $model, $version, $descp, $eventdata);

        if ($status) {
            try
            {
                $av_plugin   = new Av_plugin();
                $plugin_name = "custom_" . $pgname;
                $nsids = count($eventdata);
                $file_output = $av_plugin->save_plugin($plugin_name,$pgid,$vendor,$model,$version,$prodType,$nsids);
                $data['status'] = "OK";
                $data["data"] = $file_output;
            }
            catch(Exception $e)
            {
                $data['status'] = "Error";
                $data["data"] = $e->getMessage();
                // $config_nt['content'] = $e->getMessage().$back_link;
                
                // $nt = new Notification('nt_1', $config_nt);
                // $nt->show();
                
                // die();
            }

            
        } else {
            $data['status'] = "Error";
            $data["data"] = "Unable to generate plugin";
        }
    } else {
        $data['status'] = "Error";
        $data["data"] = "Invalid data";
    }
    echo json_encode($data);
} else {
    $data['status'] = "Error";
    $data["data"] = "Invalid action";
    echo json_encode($data);
}

function generate_plugin($conn, $pgid, $pgname, $prodType, $vendor, $model, $version, $descp, $eventdata)
{
    $pluginFilename = "custom_" . $pgname . ".cfg";
    $sqlFilename = "custom_" . $pgname . ".sql";
    $logf = explode(".", $pluginFilename);
    $logFilename = $logf[0] . ".log";
    $fp = fopen("/usr/share/ossim/www/av_plugin/views/plugin_builder/upload/" . $pluginFilename, "w");
    
    if ($fp == false) {
        return false;
    } else {

        $content = "# NCCS Proprietary plugin\n# Author: NCCS Team at plugins@NCCS.com\n# Plugin " . $pgname . " id:" . $pgid . " version: " . $version . "\n# Description:
# " . $descp . "
#
#
#

[DEFAULT]
plugin_id=" . $pgid . "\n\n
[config]
type=detector
enable=true
source=log
location=/var/log/customlog/" . $logFilename . "
create_file=true
process=
start=
stop=
    
    
";
        $var = json_decode($eventdata);

        $eventquerydata = array();
        if (sizeof($var) > 0) {
            // print_r($var);
            $i = 0;
            foreach ($var as $data) {
                $sid = $i + 1;
                $content .= "[00" . $sid . " - " . $data->eventname . "]\n";
                $content .= $data->eventType . "\n";
                $content .= $data->regex . "\n";
                $content .= "plugin_sid=" . $sid . "\n";
                if ($data->date != "") {
                    $content .= $data->date . "\n";
                }
                if ($data->device != "") {
                    $content .= $data->device . "\n";
                }
                if ($data->src_ip != "") {
                    $content .= $data->src_ip . "\n";
                }
                if ($data->dst_ip != "") {
                    $content .= $data->dst_ip . "\n";
                }
                if ($data->src_port != "") {
                    $content .= $data->src_port . "\n";
                }
                if ($data->dst_port != "") {
                    $content .= $data->dst_port . "\n";
                }
                if ($data->username != "") {
                    $content .= $data->username . "\n";
                }
                if ($data->filename != "") {
                    $content .= $data->filename . "\n";
                }
                if ($data->userdata1 != "") {
                    $content .= $data->userdata1 . "\n";
                }
                if ($data->userdata2 != "") {
                    $content .= $data->userdata2 . "\n";
                }
                if ($data->userdata3 != "") {
                    $content .= $data->userdata3 . "\n";
                }
                if ($data->userdata4 != "") {
                    $content .= $data->userdata4 . "\n";
                }
                if ($data->userdata5 != "") {
                    $content .= $data->userdata5 . "\n";
                }
                if ($data->userdata6 != "") {
                    $content .= $data->userdata6 . "\n";
                }
                if ($data->userdata7 != "") {
                    $content .= $data->userdata7 . "\n";
                }

                $content .=  "\n";
                $eventquerydata[$i] = [$pgid, $sid, $data->category, $data->classification, $pgname . ":" . $data->eventname, $data->priority, $data->reliablity, $data->subcategory];
                $i++;
            }
        }
        $state = file_put_contents("/usr/share/ossim/www/av_plugin/views/plugin_builder/upload/" . $pluginFilename, $content, FILE_APPEND | LOCK_EX);

        fclose($fp);
        $type = 1;

        $sqlcontent = "INSERT INTO alienvault.plugin (ctx,id, type, name, description,vendor,product_type) VALUES ('','$pgid','1','$pgname','$descp','$vendor','$prodType');";

        
        $nsids=0;
        foreach ($eventquerydata as $qdata) {

            $sqlcontent .= "INSERT INTO alienvault.plugin_sid (plugin_id, sid, category_id, class_id, name,priority, reliability,subcategory_id) VALUES ('$qdata[0]','$qdata[1]','$qdata[2]','$qdata[3]','$qdata[4]','$qdata[5]','$qdata[6]','$qdata[7]');";
            $nsids=$nsids+1;
        }

        $sqlstate = file_put_contents("/usr/share/ossim/www/av_plugin/views/plugin_builder/upload/" .$sqlFilename, $sqlcontent, FILE_APPEND | LOCK_EX);
        
        return true;
    }
}
$db->close();

