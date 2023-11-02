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
?>
<html>
<head>
    <link rel="stylesheet" href="css/accordion_css.css">
</head>
<body>
    <div class="" id="TokenizationModalCenter" tabindex="-1" role="dialog" aria-labelledby="exampleModalCenterTitle" data-keyboard="false" data-backdrop="static">
        <div class="modal-dialog modal-dialog-centered modal-xl" role="document">
            <div class="modal-content">
                <!-- <div class="modal-header">
               <h5 class="modal-title" id="exampleModalLongTitle">Tokenization</h5>
                </div> -->
                <div class="modal-body">
                    <div class="container">
                        <table class="table plgn_mdl_tbl">
                            <tbody>
                                <tr>
                                    <td>
                                        <labelx for="">
                                            Event Name
                                        </label>
                                        <input type="text" class="form-control" id="txtEventName" placeholder="Event Name" required>
                                        <span id="errEventName"></span>
                                    </td>
                                    <td>
                                        <label for="">
                                            Class
                                        </label>
                                        <select name="txtClassification" id="txtClassification" class="form-control">
                                            <option value="none">Select Class</option>
                                            <?php echo $cls ?>
                                        </select>
                                    </td>
                                </tr>
                                <tr>
                                    <td>
                                        <label for="">
                                            Category
                                        </label>
                                        <select name="txtCategory" id="txtCategory" class="form-control">
                                            <option value="none">Select Category</option>
                                            <?php echo $cats ?>
                                        </select>
                                    </td>
                                    <td>
                                        <label for="">
                                            Reliability
                                        </label>
                                        <select name="txtReliability" id="txtReliability" class="form-control">
                                            <option value="0">0</option>
                                            <option value="1">1</option>
                                            <option value="2">2</option>
                                            <option value="3">3</option>
                                            <option value="4">4</option>
                                            <option value="5">5</option>
                                            <option value="6">6</option>
                                            <option value="7">7</option>
                                            <option value="8">8</option>
                                            <option value="9">9</option>
                                            <option value="10">10</option>
                                        </select>
                                    </td>
                                </tr>
                                <tr>
                                    <td>
                                        <label for="">
                                            Subcategory
                                        </label>
                                        <select name="txtSubCategory" id="txtSubCategory" class="form-control">
                                            <option value="none">Select Sub Category</option>
                                        </select>
                                    </td>
                                    <td>
                                        <label for="">
                                            Priority
                                        </label>
                                        <select name="txtPriority" id="txtPriority" class="form-control">
                                            <option value="0">0</option>
                                            <option value="1">1</option>
                                            <option value="2">2</option>
                                            <option value="3">3</option>
                                            <option value="4">4</option>
                                            <option value="5">5</option>
                                        </select>
                                    </td>
                                </tr>
                                <tr>
                                    <td class="logLine_td" colspan="2">
                                        <div id="logLine" row="1" class="text-wrap"><?php echo "<script> const mode = sessionStorage.getItem('log_value'); document.write(mode); </script>"; ?></div>
                                        <div id="result_err"></div>
                                    </td>
                                </tr>
                                <tr>
                                    <td class="logLine_td" colspan="2">
                                        <label for="">Selection</label>
                                        <div id="result"></div>
                                        <div id="clear_results" style="user-select:text">[X]</div>
                                    </td>
                                </tr>
                                <tr>
                                    <td>
                                        <label for="">
                                            Data field Assigned
                                        </label>
                                        <select name="" id="fieldSelection">
                                            <option value="" selected disabled hidden>Choose field</option>
                                            <option value="date">date</option>
                                            <option value="time">time</option>
                                            <option value="device">device</option>
                                            <option value="src_ip">src_ip</option>
                                            <option value="dst_ip">dst_ip</option>
                                            <option value="src_port">src_port</option>
                                            <option value="dst_port">dst_port</option>
                                            <option value="username">username</option>
                                            <option value="filename">filename</option>
                                            <option value="userdata1">userdata1</option>
                                            <option value="userdata2">userdata2</option>
                                            <option value="userdata3">userdata3</option>
                                            <option value="userdata4">userdata4</option>
                                            <option value="userdata5">userdata5</option>
                                            <option value="userdata6">userdata6</option>
                                            <option value="userdata7">userdata7</option>
                                        </select>
                                    </td>
                                    <td class="td_center assign_btn_div">
                                        <!-- <button id="btnAssign" class="btn btn-warning m_btn" onclick="assign_btn()">Assign</button> -->
                                        <button id="btnAssign" class="btn btn-warning m_btn" onclick="tokenization()" disabled>Assign</button>
                                    </td>
                                </tr>
                                <tr>
                                    <td class="td_center td_opts ">
                                        <button type="button" class="btn btn-secondary m_btn" data-dismiss="modal" onclick="close_modal()">Close</button>
                                    </td>
                                    <td class="td_center td_opts ">
                                        <button type="button" class="btn btn-primary btnSaveEvent m_btn" onclick="save_modal_data()">Save changes</button>
                                    </td>
                                </tr>
                            </tbody>
                        </table>
                    </div>
                </div>
            </div>
        </div>
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

    function getSelectionText() {
        try {
            var text = window.getSelection().getRangeAt(0);
            if (!text) {
                console('Your selection text is: ufd');
            } else {
                return text;
            }


        } catch (err) {}
    }
    let showText = document.getElementById("result");
    let selected = document.getElementById("logLine");
function getSelectedText() {
    var selectedText = "";
    showText.innerHTML = "";

    if (window.getSelection) {
    selectedText = window.getSelection().toString();
    showText.innerHTML = selectedText;
    }
}
    selected.onmouseup = selected.onkeyup = selected.onselectionchange = function() {

        // document.getElementById("result").innerText = getSelectionText();
        getSelectedText();
        if($("#result_err").text() != "") {
            $("#result_err").text("");
        }

        if($('#fieldSelection').val()){
           $("#btnAssign").prop("disabled", false);
        }else{
            $("#btnAssign").prop("disabled", true);
        }
    };

    $("#txtCategory").change(function() {
        var catid = $("#txtCategory").val();
        if (catid > 0) {
            var fd = new FormData();
            fd.append("catid", catid);
            fd.append("action", "fetchsubcat");
            $.ajax({
                url: "action.php",
                type: "post",
                data: fd,
                contentType: false,
                processData: false,
                success: function(response) {
                    var data = JSON.parse(response);
                    if (data["status"] == "OK") {
                        $("#txtSubCategory").empty();
                        $("#txtSubCategory").append(`<option value="none">Select Sub Category</option>`);
                        $("#txtSubCategory").append(data["data"]);
                    }
                }
            });
        } else {
            $("#txtSubCategory").empty();
            $("#txtSubCategory").append(`<option value="none">Select Sub Category</option>`);
        }
    });

</script>


</html>
