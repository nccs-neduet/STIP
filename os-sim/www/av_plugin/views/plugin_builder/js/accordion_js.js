class arrayProcessing {
    static orignalArray;
    static counter;
    static state;
    static uniqueLogandRegex = [];
}
$(".plgn_mdl_tbl").css("user-select","none");
$("#logLine").css("user-select","text");
// Making accordion functionable. (It has stopped working somehow. Will fix it later)
$(document).ready(function () {
    $(".loader-ovrd").hide();
    $(".plgn_accord_body").fadeOut(0);
    step1();
    $("#mxContainer").not($("#mxContainer")).slideUp('fast');
    $(".plgn_accord_body").not($(".plgn_accord_body")).slideUp('fast');



    $(".plgn_accord_body").slideToggle(400);
    // window.sessionStorage.clear();

});

// Defined method to close gb_show()
function close_modal() {
    window.sessionStorage.removeItem('mode');
    parent.window.location.reload();

}

// Showing upload file button and validating file on upload
function step1() {
    $('#mxContainer').html(`<div class="upload_btn">
    <div class="pluginContainer">
        <label>Upload log file:</label>
        <input type="file" id="iplogFile" name="iplogFile" class="" required
            accept=".log" />
        <button id="btnNext" type="button" class="">Next</button>
        <span id="err_msg"></span>
    </div>
</div>`);

    $('#btnNext').click(function () {
        $(".loader-ovrd").show();
        var fd = new FormData();
        var files = $('#iplogFile')[0].files;

        if (files.length > 0) {
            fd.append('file', files[0]);
            fd.append('action', 'logUpload');
            $.ajax({
                url: 'views/plugin_builder/action.php',
                type: 'post',
                data: fd,
                contentType: false,
                processData: false,
                success: function (response) {
                    var data = JSON.parse(response);
                    if (data['status'] == 'OK') {
                        step2(data);
                    } else {
                        $("#err_msg").text("*Invalid Log File");
                        $(".loader-ovrd").hide();

                    }
                }
            });
        } else {
            $(".loader-ovrd").hide();
            $("#err_msg").text("*Log File Required");
        }
    });

}

function step2(data) {

    var entry = data["data"][0];
    $('#rg_raw_input_text').val(entry).trigger("input");
    arrayProcessing.orignalArray = data["data"];
    arrayProcessing.counter = 0;
    arrayProcessing.state = false;
    setTimeout(function () {
        genRegex();
    }, 2000);

}

/**
 * Description. This function generates the regex of log line by accessing log from the array defined in arrayProcessing class named as "orignalArray". it generates the regex then remove
 * all the entries from the array that match the patteren generated and store the unique log and their regex into a 2D array defined in arrayProcessing class named 
 * as uniqueLogandRegex. 
 */
function genRegex() {
    var q = 1;
    LABEL1: do {

        if (arrayProcessing.state) {
            required_items2ndlvl = document.querySelectorAll(".rg-match-row:nth-child(2) .rg-match-item ");
            for (j = 0; j < required_items2ndlvl.length; j++) {
                var a = required_items2ndlvl[j].getElementsByTagName('a');
                if (a.length > 1) {
                    for (k = 0; k < a.length; k++) {
                        if (a[k].innerText == "Multiple characters") {
                            a[k].click();
                            break;
                        }
                        if (a[k].innerText == "Number") {
                            a[k].click();
                            break;
                        }
                        if (a[k].innerText == "Time") {
                            a[k].click();
                            break;
                        }
                        if (a[k].innerText == "Date") {
                            a[k].click();
                            break;
                        }
                        if (a[k].innerText == "IPv4 address") {
                            a[k].click();
                            break;
                        }
                        if (a[k].innerText == "Parentheses") {
                            a[k].click();
                            break;
                        }
                        if (a[k].innerText == "Square brackets") {
                            a[k].click();
                            break;
                        }

                    }
                } else {
                    if (a[0].innerText == "Multiple characters") {
                        a[0].click();
                    }
                }
            }

            required_items3rdlvl = document.querySelectorAll(".rg-match-row:nth-child(3) .rg-match-item:not(.rg-item-not-available)");
            for (j = 0; j < required_items3rdlvl.length; j++) {
                var b = required_items3rdlvl[j].getElementsByTagName('a');
                if (b.length > 1) {
                    for (k = 0; k < b.length; k++) {
                        if (b[k].innerText == "Number") {
                            b[k].click();
                            break;
                        }
                        if (b[k].innerText == "Alphanumeric characters") {
                            b[k].click();
                            break;
                        }
                        if (b[k].innerText == "Multiple characters") {
                            b[k].click();
                            break;
                        }
                    }
                } else {
                    if (b[0].innerText == "Multiple characters") {
                        b[0].click();
                    }
                }
            }

            required_items4thlvl = document.querySelectorAll(".rg-match-row:nth-child(4) .rg-match-item:not(.rg-item-not-available)");
            if (required_items4thlvl.length > 0) {
                for (j = 0; j < required_items4thlvl.length; j++) {
                    var b = required_items4thlvl[j].getElementsByTagName('a');
                    if (b.length > 1) {
                        for (k = 0; k < b.length; k++) {
                            if (b[k].innerText == "Number") {
                                b[k].click();
                                break;
                            }
                            if (b[k].innerText == "Alphanumeric characters") {
                                b[k].click();
                                break;
                            }
                            if (b[k].innerText == "Multiple characters") {
                                b[k].click();
                                break;
                            }

                        }
                    } else {
                        if (b[0].innerText == "Multiple characters") {
                            b[0].click();
                        }
                    }
                }
            }

            required_items5thlvl = document.querySelectorAll(".rg-match-row:nth-child(5) .rg-match-item:not(.rg-item-not-available)");
            if (required_items5thlvl.length > 0) {
                for (j = 0; j < required_items5thlvl.length; j++) {
                    var b = required_items5thlvl[j].getElementsByTagName('a');
                    if (b.length > 1) {
                        for (k = 0; k < b.length; k++) {
                            if (b[k].innerText == "Number") {
                                b[k].click();
                                break;
                            }
                            if (b[k].innerText == "Multiple characters") {
                                b[k].click();
                                break;
                            }
                        }
                    } else {
                        if (b[0].innerText == "Multiple characters") {
                            b[0].click();
                        }
                    }
                }
            }

            required_items6thlvl = document.querySelectorAll(".rg-match-row:nth-child(6) .rg-match-item:not(.rg-item-not-available)");
            if (required_items6thlvl.length > 0) {
                for (j = 0; j < required_items6thlvl.length; j++) {
                    var b = required_items6thlvl[j].getElementsByTagName('a');
                    if (b.length > 1) {
                        for (k = 0; k < b.length; k++) {
                            if (b[k].innerText == "Number") {
                                b[k].click();
                                break;
                            }
                            if (b[k].innerText == "Multiple characters") {
                                b[k].click();
                                break;
                            }
                        }
                    } else {
                        if (b[0].innerText == "Multiple characters") {
                            b[0].click();
                        }
                    }
                }
            }

            required_items7thlvl = document.querySelectorAll(".rg-match-row:nth-child(7) .rg-match-item:not(.rg-item-not-available)");
            if (required_items7thlvl.length > 0) {
                for (j = 0; j < required_items7thlvl.length; j++) {
                    var b = required_items7thlvl[j].getElementsByTagName('a');
                    if (b.length > 1) {
                        for (k = 0; k < b.length; k++) {
                            if (b[k].innerText == "Number") {
                                b[k].click();
                                break;
                            }
                            if (b[k].innerText == "Multiple characters") {
                                b[k].click();
                                break;
                            }
                        }
                    } else {
                        if (b[0].innerText == "Multiple characters") {
                            b[0].click();
                        }
                    }
                }
            }

            required_items8thlvl = document.querySelectorAll(".rg-match-row:nth-child(7) .rg-match-item:not(.rg-item-not-available)");
            if (required_items8thlvl.length > 0) {
                for (j = 0; j < required_items8thlvl.length; j++) {
                    var b = required_items8thlvl[j].getElementsByTagName('a');
                    if (b.length > 1) {
                        for (k = 0; k < b.length; k++) {
                            if (b[k].innerText == "Number") {
                                b[k].click();
                                break;
                            }
                            if (b[k].innerText == "Multiple characters") {
                                b[k].click();
                                break;
                            }
                        }
                    } else {
                        if (b[0].innerText == "Multiple characters") {
                            b[0].click();
                        }
                    }
                }
            }

            break LABEL1;

        } else {
            continue LABEL1;
        }

    } while (1);

    var regexp = "";
    $("#rg_result_display span").each(function (index) {
        regexp += $(this).text();
    });

    arrayProcessing.uniqueLogandRegex.push([$('#rg_raw_input_text').val(), regexp]);

    removeItem(regexp);
    $('.rg-item-selected').trigger('click');

    var entry = arrayProcessing.orignalArray[0];
    $('#rg_raw_input_text').val("").trigger("input");
    arrayProcessing.state = false;
    $('#rg_raw_input_text').val(entry).trigger("input");
    setTimeout(function () {
        if (arrayProcessing.orignalArray.length > 0) {
            genRegex();
        } else {

            // REMOVING DUPLICATION
            var el = arrayProcessing.uniqueLogandRegex;
            function multiDimensionalUnique(el) {
                var uniques = [];
                var itemsFound = {};
                for (var i = 0, l = el.length; i < l; i++) {
                    var stringified = JSON.stringify(el[i]);
                    if (itemsFound[stringified]) { continue; }
                    uniques.push(el[i]);
                    itemsFound[stringified] = true;
                }
                return uniques;
            }
            //END REMOVING DUPLICATION
            var fd = new FormData();
            fd.append('regexs', JSON.stringify(multiDimensionalUnique(el)));
            fd.append('action', 'regexGenerated');
            $.ajax({
                url: 'views/plugin_builder/action.php',
                type: 'post',
                data: fd,
                contentType: false,
                processData: false,
                beforeSend: function () {
                    // $(".loader-ovrd").show();
                },
                success: function (response) {
                    var data = JSON.parse(response);
                    if (data['status'] == 'OK') {
                        $(".loader-ovrd").hide();
                        step3(data);

                    } else {
                        $(".loader-ovrd").hide();
                        alert('Something went wrong');

                    }
                }

            });
        }
    }, 1000);

}

/* my added code */
$(document)
    .ajaxStart(function () {
        $(".loader-ovrd").show();
    });



function removeItem(itemToRemove) {
    arrayProcessing.orignalArray.splice(0, 1);
    arrayProcessing.counter++;
}
var arr;
var colselection;

// Adding Greybox
function toggle_modal(e) {
    // e.stopPropagation();
    window.sessionStorage.setItem("arr_items2", "12345");
    var row = $(e).parent().parent();

    colselection = row.find("td:nth-child(4)");
    if (colselection[0].innerHTML != "Done") {

        var log = $(e).attr("data-log");
        var r = $(e).attr("data-regex");
        window.sessionStorage.setItem('log_value', log);
        // window.sessionStorage.setItem('element_value', colselection[0]);
        var reg = String.raw`${r}`;

        var logArr = log.split(' ');
        var regArr = reg.split(' ');

        arr = getArray(logArr, regArr);
        window.sessionStorage.setItem("arr_items", JSON.stringify(arr));

       
        var t = $(e).attr('data-title');
        var href = $(e).attr('href');
        GB_show(t, href, 650, '40%');

    } else {
        alert("Event already saved");
    }
    return false;
}
$(document).on('click', '.deleteModalEntry', function () {
    //Event Delete code goes here
    var row = $(this).parent().parent();
    let text = "Do you really want to remove this event?";
    if (confirm(text) == true) {
        row.remove();
    }

});

$(document).on('click', '#btnFinalNext', function () {

    var checking = $('#tblEvent tr>td:last-child');
    var flag = 1;
    for (var i = 0; i < checking.length; i++) {
        if (checking[i].innerHTML != "Done") {
            flag = 0;
        }
    }
    if (flag == 1) {

        

        var plgn_keys = new Array(
            "plugiName",
            "pluginVendor",
            "pluginModel",
            "pluginVersion",
            "pluginProductType",
            "pluginEventCount");


        var plgn_details = new Array(
            "/custom_" + $("#txtPluginName").val() + ".cfg",
            $('#txtVendor').val(),
            $('#txtModel').val(),
            $('#txtVersion').val(),
            $('#txtProductType option:selected').text(),
            checking.length);
    

        var plgn_final_keys = new Array(
            "pgid",
            "pgname",
            "prodType",
            "vendor",
            "model",
            "version",
            "descp");

        var plgn_final_data = new Array(
            $("#txtPluginId").val(),
            $("#txtPluginName").val(),
            $("#txtProductType").val(),
            plgn_details[1],
            plgn_details[2],
            plgn_details[3],
            $("#txtDescription").val()
        );
        var plgn_arr = getArray(plgn_keys, plgn_details);
        var plgn_final_arr = getArray(plgn_final_keys, plgn_final_data);

        window.sessionStorage.setItem('plugin_details', JSON.stringify(plgn_arr));
        window.sessionStorage.setItem('final_plugin_details', JSON.stringify(plgn_final_arr));

        var t = $(this).attr('data-title');
        var href = $(this).attr('href');
        GB_show(t, href, 470, '40%');

    } else {
        alert('Configure pending events/Delete pending events');
    }
});

$(document).on('click', '#btnPluginFinish', function () {
    var fd = new FormData();

    // var pgid = $("#txtPluginId").val();
    // var pgname = $("#txtPluginName").val();
    // var prodType = $("#txtProductType").val();
    // var vendor = $("#txtVendor").val();
    // var model = $("#txtModel").val();
    // var version = $("#txtVersion").val();
    // var descp = $("#txtDescription").val();

    fd.append("action", "generateplugin");
    var arr = JSON.parse(sessionStorage.getItem("final_plugin_details"));
   
    var eventsData = JSON.parse(window.sessionStorage.getItem('eventsData'));
    // window.sessionStorage.removeItem("final_plugin_details");
    window.sessionStorage.removeItem("eventsData");
   
    for (i = 0; i < arr.length; i++) {
        
        fd.append(arr[i][0], arr[i][1]);

    }
    var jsondata = JSON.stringify(eventsData);
    fd.append("eventdata", jsondata);
    $.ajax({
        url: "action.php",
        type: "post",
        data: fd,
        contentType: false,
        processData: false,
        success: function (response) {
            var data = JSON.parse(response);
            if (data["status"] == "OK") {
                console.log(data);
                window.sessionStorage.clear();            
                close_modal();
                console.log(0);
                window.location.reload();
                document.getElementById("back_button").click();
                console.log(1);
                
                
                //$("#add_button").click();
            }
        }
    });
});
/***Final phase */
class pluginEvent {
    eventType = "";
    eventname = "";
    category = "";
    subcategory = "";
    classification = "";
    priority = "";
    reliablity = "";
    regex = "";
    date = "";
    time = "";
    device = "";
    src_ip = "";
    dst_ip = "";
    src_port = "";
    dst_port = "";
    username = "";
    filename = "";
    userdata1 = "";
    userdata2 = "";
    userdata3 = "";
    userdata4 = "";
    userdata5 = "";
    userdata6 = "";
    userdata7 = "";
}
var eventsData = [];

function save_modal_data() {

    var eventname = $("#txtEventName").val();
    var category = $("#txtCategory").val();
    var subcategory = $("#txtSubCategory").val();
    var classification = $("#txtClassification").val();
    var priority = $("#txtPriority").val();
    var reliablity = $("#txtReliability").val();
    if (eventname == "") {

        $("#errEventName").text(`Event Name is required`);
        $("#errEventName").attr("class", "text-danger");
        return;

    } else {
        $("#errEventName").text(``);
        $("#errEventName").attr("class", "");
    }
    eventDetail.eventType = "event_type=event";
    eventDetail.eventname = eventname;
    eventDetail.category = category;
    eventDetail.subcategory = subcategory;
    eventDetail.classification = classification;
    eventDetail.priority = priority;
    eventDetail.reliablity = reliablity;
    // if(eventDetail.regex.length === 0)
    // {
    var regex = "";
    var arr = JSON.parse(sessionStorage.getItem("arr_items"));
  
    for (i = 0; i < arr.length; i++) {
        if (i != arr.length - 1) {
            if (arr[i + 1][0] == "") {
                regex = regex + arr[i][1] + " ";
            } else if (arr[i][0] == "/") {
                var tlog = arr[i][1];
                tlog = [tlog.slice(0, tlog.length - 1), "\\", tlog.slice(tlog.length - 1)].join('');
                regex = regex + tlog;
            } else {
                regex = regex + arr[i][1];
            }


        } else {
            regex = regex + arr[i][1];
        }
    }
    eventDetail.regex = 'regexp="' + regex + '"';
    //}
    window.parent[0].colselection[0].innerHTML = "Done";
    eventsData.push(eventDetail);
    var oldItems = JSON.parse(sessionStorage.getItem('eventsData')) || [];
    oldItems.push(eventDetail);
    window.sessionStorage.setItem('eventsData', JSON.stringify(oldItems));
    eventDetail = new pluginEvent();
    $("#txtEventName").val('');
    $("#txtCategory").val('NULL').trigger('change');
    $("#txtSubCategory").val('NULL').trigger('change');
    $("#txtClassification").val('NULL').trigger('change');
    $("#txtReliability").val('0').trigger('change');
    $("#txtPriority").val('0').trigger('change');
    close_modal();

}

/*---------------Tokenization Script---------------*/

var eventDetail = new pluginEvent();

function tokenization() {
    
    var txt = $('#result').text();
    var txtArr = txt.split(' ');
    var startIndex = -1, endIndex = -1;
 
    var arr = JSON.parse(sessionStorage.getItem("arr_items"));
    for (i = 0; i < arr.length; i++) {
        if (arr[i][0] == txtArr[0] && startIndex == -1) {
            startIndex = i;
        }
        if (arr[i][0] == txtArr[txtArr.length - 1]) {
            endIndex = i;
        }
    }
    if (startIndex == -1 || endIndex == -1) {
        alert("Invalid Selection");
        return;
    }

    var opt = $("#fieldSelection option:selected").text();

    if (opt == "date") {
        arr[startIndex][1] = "(?P<date>" + arr[startIndex][1];
        arr[endIndex][1] = arr[endIndex][1] + ")";
    } else if (opt == "time") {
        arr[startIndex][1] = "(?P<time>" + arr[startIndex][1];
        arr[endIndex][1] = arr[endIndex][1] + ")";
    } else if (opt == "device") {
        arr[startIndex][1] = "(?P<device>" + arr[startIndex][1];
        arr[endIndex][1] = arr[endIndex][1] + ")";
    } else if (opt == "src_ip") {
        arr[startIndex][1] = "(?P<src>" + arr[startIndex][1];
        arr[endIndex][1] = arr[endIndex][1] + ")";
    } else if (opt == "dst_ip") {
        arr[startIndex][1] = "(?P<dst>" + arr[startIndex][1];
        arr[endIndex][1] = arr[endIndex][1] + ")";
    } else if (opt == "src_port") {
        arr[startIndex][1] = "(?P<src_port>" + arr[startIndex][1];
        arr[endIndex][1] = arr[endIndex][1] + ")";
    } else if (opt == "dst_port") {
        arr[startIndex][1] = "(?P<dst_port>" + arr[startIndex][1];
        arr[endIndex][1] = arr[endIndex][1] + ")";
    } else if (opt == "username") {
        arr[startIndex][1] = "(?P<user>" + arr[startIndex][1];
        arr[endIndex][1] = arr[endIndex][1] + ")";
    } else if (opt == "filename") {
        arr[startIndex][1] = "(?P<filename>" + arr[startIndex][1];
        arr[endIndex][1] = arr[endIndex][1] + ")";
    } else if (opt == "userdata1") {
        arr[startIndex][1] = "(?P<userdata1>" + arr[startIndex][1];
        arr[endIndex][1] = arr[endIndex][1] + ")";
    } else if (opt == "userdata2") {
        arr[startIndex][1] = "(?P<userdata2>" + arr[startIndex][1];
        arr[endIndex][1] = arr[endIndex][1] + ")";
    } else if (opt == "userdata3") {
        arr[startIndex][1] = "(?P<userdata3>" + arr[startIndex][1];
        arr[endIndex][1] = arr[endIndex][1] + ")";
    } else if (opt == "userdata4") {
        arr[startIndex][1] = "(?P<userdata4>" + arr[startIndex][1];
        arr[endIndex][1] = arr[endIndex][1] + ")";
    } else if (opt == "userdata5") {
        arr[startIndex][1] = "(?P<userdata5>" + arr[startIndex][1];
        arr[endIndex][1] = arr[endIndex][1] + ")";
    } else if (opt == "userdata6") {
        arr[startIndex][1] = "(?P<userdata6>" + arr[startIndex][1];
        arr[endIndex][1] = arr[endIndex][1] + ")";
    } else if (opt == "userdata7") {
        arr[startIndex][1] = "(?P<userdata7>" + arr[startIndex][1];
        arr[endIndex][1] = arr[endIndex][1] + ")";
    }

    window.sessionStorage.setItem("arr_items", JSON.stringify(arr));
    /********************* Need an else there for the validation of selection of opt IF NEEDED ***********************/
    // var regex = "";
    // for (i = 0; i < arr.length; i++) {
    //     if (i != arr.length - 1) {
    //         if (arr[i + 1][0] == "") {
    //             regex = regex + arr[i][1] + " ";
    //         } else if (arr[i + 1][0] == "/") {
    //             regex = regex + arr[i][1] + "\\";
    //         } else {
    //             regex = regex + arr[i][1];
    //         }


    //     } else {
    //         regex = regex + arr[i][1];
    //     }
    // }

    // eventDetail.regex = 'regexp="' + regex + '"';
    if (opt == "date") {
        eventDetail.date = "date={normalize_date($date)}";
    } else if (opt == "device") {
        eventDetail.device = "device={$device}";
    } else if (opt == "time") {
        eventDetail.time = "device={$time}";
    } else if (opt == "src_ip") {
        eventDetail.src_ip = "src_ip={$src}";
    } else if (opt == "dst_ip") {
        eventDetail.dst_ip = "dst_ip={$dst}";
    } else if (opt == "src_port") {
        eventDetail.src_port = "src_port={$src_port}";
    } else if (opt == "dst_port") {
        eventDetail.dst_port = "dst_port={$dst_port}";
    } else if (opt == "username") {
        eventDetail.username = "username={$user}";
    } else if (opt == "filename") {
        eventDetail.filename = "filename={$filename}";
    } else if (opt == "userdata1") {
        eventDetail.userdata1 = "userdata1={$userdata1}";
    } else if (opt == "userdata2") {
        eventDetail.userdata2 = "userdata2={$userdata2}";
    } else if (opt == "userdata3") {
        eventDetail.userdata3 = "userdata3={$userdata3}";
    } else if (opt == "userdata4") {
        eventDetail.userdata4 = "userdata4={$userdata4}";
    } else if (opt == "userdata5") {
        eventDetail.userdata5 = "userdata5={$userdata5}";
    } else if (opt == "userdata6") {
        eventDetail.userdata6 = "userdata6={$userdata6}";
    } else if (opt == "userdata7") {
        eventDetail.userdata7 = "userdata7={$userdata7}";
    }

    const selection = window.getSelection();
    if (selection && selection.rangeCount > 0) {
        const range = selection.getRangeAt(0).cloneRange()

        if (range.endOffset - range.startOffset > 0) {
            const span = document.createElement('span');
            span.setAttribute('style', 'color: red; backgroundColor: transparent;');
            span.setAttribute('class', 'noselect');
            span.setAttribute('title', 'Date');
            range.surroundContents(span);
            selection.removeAllRanges();
        }
    }
    $("#fieldSelection option:selected").hide();
    $('#btnAssign').prop('disabled', true);
    $("#result").text('');
    $("#fieldSelection").val('').trigger('change');
    var s
    if($("#logLine:contains(${txt})").length > 0){
        $("#header").css("background", "red");
    }

}

function getArray(table1, table2) {
    var i, out = []; //literal new array
    for (i = 0; i < table1.length; i++) {
        out.push([table1[i], table2[i]]);
    }
    return out;
}
/**
 * Tokenization Script Ends here
 * **/
function step3(data) {
    $('#mxContainer').html(data['data']);
}

function check_plgn_name(obj) {
    var pluginname = obj.value;

    if (pluginname != "") {
        var fd = new FormData();
        fd.append("plugin_name", pluginname);
        fd.append("action", "checkplugname");
        $.ajax({
            url: "views/plugin_builder/action.php",
            type: "POST",
            data: fd,
            contentType: false,
            processData: false,
            success: function (response) {
                var data = JSON.parse(response);
                if (data["status"] == "OK") {
                    $("#availability").text('Plugin name Available');
                    $("#availability").attr("style", "color:#28a745!important;position:absolute; right:24px; bottom:-14px;");
                }
                else {
                    $("#availability").text('Plugin with this name already exist');
                    $("#availability").attr("style", "color:#dc3545!important;position:absolute; right:24px; bottom:-14px;");
                }
            },
            error: function (err) {
                alert(err);
            }
        });
    }
    else {
        $("#availability").text(``);
        $("#availability").attr("class", "");
    }
}

$(document).on('click', '#clear_results', function () {

    $('#result').empty();
});

$('#fieldSelection').change(function () {

    if ($("#result").text() != '') {
        $('#btnAssign').prop('disabled', false);
    } else {
        if ($('#fieldSelection').val() != '') {
            $("#result_err").text("Please select the required parameters.");
        } else {
            $("#result_err").text("");
        }

    }

});

