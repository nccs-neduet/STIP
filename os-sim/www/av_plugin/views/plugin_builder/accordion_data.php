<div class="plgn_accord_cont">
    <div class="plgn_accord_body" id="accordion">
        <div id="mxContainer">

            <script>
           /** ------------Category dropdown selection change event start------------**/
                $("#txtCategory").change(function() {
                    var catid = $("#txtCategory").val();
                    if (catid > 0) {
                        var fd = new FormData();
                        fd.append("catid", catid);
                        fd.append("action", "fetchsubcat");
                        $.ajax({
                            url: "views/plugin_builder/action.php",
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
                /** ------------Category dropdown selection change event end------------**/

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
            </script>

        </div>
    </div>
</div>
<div id="tester" class="loader loader-ovrd"></div>


<!--Container for regex generator-->
<div id="regexContainer" style="display: none;">
    <div id="rg_mainContainer">

        <div id="rg_input_container">
            <div>
                <h3>1</h3>
            </div>
            <div>
                <div>
                    <h5>Paste a text sample.</h5>
                    <a id="rg_button_show_help" title="Show help">&#10068;</a>
                </div>
                <input type="text" maxlength="1000" id="rg_raw_input_text" aria-label="Example text" aria-describedby="rg_raw_text_input_help" placeholder="Text here..." autofocus="autofocus" value="">
                <div id="rg_raw_input_message_shorten" role="alert">
                    <small>We have shortened the input to <span id="rg_raw_input_message_shorten_number">1000</span> characters to speed up
                        processing.</small>
                </div>
                <small id="rg_raw_text_input_help">Give us an example of
                    the text you want
                    to
                    match using your <em>regex</em>. We will provide you with some ideas how to build a
                    regular
                    expression.</small>
            </div>
        </div>

        <div id="rg_pattern_selection_container">
            <div>
                <h3>2</h3>
            </div>
            <div>
                <h5>Which parts of the text are interesting for you?</h5>
                <div id="rg_result_container">
                    <div>
                        <div id="rg_text_display"></div>
                        <div id="rg_row_container">
                        </div>
                    </div>
                </div>
                <small>Click on the marked suggestions to select them for
                    your regular
                    expression.</small>
            </div>
        </div>

        <div id="rg_regex_result_container">
            <div>
                <h3>3</h3>
            </div>
            <div>
                <h5>Regular expression</h5>
                <div id="rg_result_display_box">
                    <div class="rg-match-container">
                        <div id="rg_result_display"></div>
                    </div>
                </div>
                <small>Hover the generated regular expression to see more
                    information.</small>
                <div>
                    <div>
                        <button id="rg_button_copy">Copy regex</button>
                    </div>
                    <div id="rg_div_onlymatch_container" class="col-12 col-sm-8 col-md-9 col-lg-10 mt-3 mt-sm-0 form-group">
                        <div class="card p-3">
                            <div class="form-group">
                                <div class="rg-check-row">
                                    <div class="custom-control custom-checkbox">
                                        <input type="checkbox" class="custom-control-input" id="rg_onlymatches" aria-describedby="rg_onlymatches_help">
                                        <label class="custom-control-label" for="rg_onlymatches">Generate only
                                            patterns</label>
                                    </div>
                                    <small id="rg_onlymatches_help" class="form-text text-muted mt-0">
                                        When this option is checked, the generated regular expression
                                        will only contain
                                        the patterns that you selected in step 2. Otherwise, all
                                        characters between the
                                        patterns will be copied.
                                    </small>
                                </div>
                            </div>
                            <div class="form-group">
                                <div class="rg-check-row">
                                    <div class="custom-control custom-checkbox">
                                        <input type="checkbox" class="custom-control-input" id="rg_matchwholeline" aria-describedby="rg_matchwholeline_help">
                                        <label class="custom-control-label" for="rg_matchwholeline">Match whole
                                            line</label>
                                    </div>
                                    <small id="rg_matchwholeline_help" class="form-text text-muted mt-0">
                                        Would you like the generated regular expression to match all of
                                        the input?
                                    </small>
                                </div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>

        <div class="row no-gutters bg-light mt-3 pr-4 rounded">
            <div class="col-sm-1 p-4 d-none d-sm-none d-md-block text-center">
                <h3 class="display-3 text-secondary">4</h3>
            </div>
            <div class="col-12 col-md-11 py-4 pl-4">
                <div class="card mt-2 pt-3">
                    <form>
                        <div class="form-group">
                            <div class="col-sm-12 rg-check-row">
                                <div class="custom-control custom-checkbox">
                                    <input type="checkbox" class="custom-control-input" id="rg_caseinsensitive" aria-describedby="rg_caseinsensitive_help">
                                    <label class="custom-control-label" for="rg_caseinsensitive">Case
                                        Insensitive</label>
                                </div>
                                <small id="rg_caseinsensitive_help" class="form-text text-muted mt-0">
                                    By default, all major regex engines match in case-sensitive mode. If
                                    you want
                                    patterns such as <span class="border rg-code p-1 bg-white">Name:
                                        [a-z]+</span>
                                    to match in
                                    case-insensitive fashion, we need to turn that feature
                                    on.&NonBreakingSpace;
                                    <sup><a href="#" target="_blank">*</a></sup>
                                </small>
                            </div>
                        </div>
                        <div class="form-group">
                            <div class="col-sm-12 rg-check-row">
                                <div class="custom-control custom-checkbox">
                                    <input type="checkbox" class="custom-control-input" id="rg_dotmatcheslinebreakes" aria-describedby="rg_dotmatcheslinebreakes_help">
                                    <label class="custom-control-label" for="rg_dotmatcheslinebreakes">Dot Matches
                                        Line Breaks</label>
                                </div>
                                <small id="rg_dotmatcheslinebreakes_help" class="form-text text-muted mt-0">
                                    By default, the dot <span class="border rg-code p-1 bg-white">.</span> doesn't
                                    match line break characters such as line feeds and
                                    carriage returns. If you want patterns such as <span class="border rg-code p-1 bg-white">BEGIN .*? END</span> to
                                    match across
                                    lines, we need to turn that feature on.&NonBreakingSpace;<sup><a href="#" target="_blank">*</a></sup> </small>
                            </div>
                        </div>
                        <div class="form-group">
                            <div class="col-sm-12 rg-check-row">
                                <div class="custom-control custom-checkbox">
                                    <input type="checkbox" class="custom-control-input" id="rg_multiline" aria-describedby="rg_multiline_help">
                                    <label class="custom-control-label" for="rg_multiline">Multiline</label>
                                </div>
                                <small id="rg_multiline_help" class="form-text text-muted mt-0">
                                    By default, most major engines (except Ruby), the anchors <span class="border rg-code p-1 bg-white">^</span> and <span class="border rg-code p-1 bg-white">$</span> only match
                                    (respectively) at the beginning and the end of the string.

                                    In other engines, if you want patterns such as <span class="border rg-code p-1 bg-white">^Define</span> and <span class="border rg-code p-1 bg-white">&gt;&gt;&gt;$</span> to
                                    match
                                    (respectively) at the beginning and the end of each line, we need to
                                    turn that
                                    feature on.&NonBreakingSpace;<sup><a href="#" target="_blank">*</a></sup> </small>
                            </div>
                        </div>
                    </form>
                </div>
            </div>
        </div>
        <input type="button" id="nextRegex" name="nextRegex" class="btn btn-primary my-3 float-right" value="Next Log" />

        <div class="row no-gutters bg-secondary text-light mt-3 pr-4 rounded" style="display: none;">
            <div class="col-12 py-4 pl-4">
                <div class="d-flex justify-content-between">
                    <h5>Share</h5>
                </div>
                <p>To share the current page content and settings, use the following link:</p>
                <div class="rounded border p-1">
                    <div class="rg-match-container">
                        <button class="btn btn-primary btn-sm float-right m-1" id="rg_button_copy_share_link" title="Copy link to share">&#x1F4CB</button>
                        <div id="rg_result_link" class="rg-share-link m-1"></div>
                    </div>
                </div>
            </div>
        </div>

        <!-- Footer -->
        <footer style="display: none;" class="page-footer font-small row no-gutters bg-dark text-muted border border-secondary mt-4 p-4 rounded">
            <h5 class="text-uppercase" style="width: 100%">Regex Generator</h5>
            <div class="text-center text-md-left collapse show" id="footerContent">
                <div class="row">
                    <div class="col-md-6 mt-md-0 mt-3">
                        <p>The idea for this page comes from <a href="#" target="_blank">txt2re</a>,
                            which seems to be discontinued.</p>
                        <p>The aim of this page is to give as many people as possible the opportunity to
                            develop and
                            use regular expressions. Hopefully nobody has to call <span class="rg-code">substring()</span> anymore.</p>
                        <p>Find the project sources at <a href="#" target="_blank"></a>
                        </p>
                    </div>
                    <hr class="clearfix w-100 d-md-none pb-3 border-secondary">
                    <div class="col-md-3 mb-md-0 mb-3">
                        <h5>Useful Regex Links</h5>
                        <ul class="list-unstyled">
                            <li><a href="#" target="_blank" id="rg_anchor_regex101"></a></li>
                            <li><a href="#" target="_blank" id="rg_anchor_regexr"></a></li>
                            <!-- ?expression=abc&text=def -->
                            <li><a href="#" target="_blank"></a>
                            </li>
                            <!--?regex=abc&replacement=def-->
                            <li><a href="#" target="_blank"></a></li>
                            <li><a href="#" target="_blank"></a></li>
                        </ul>
                    </div>
                    <div class="col-md-3 mb-md-0 mb-3">
                        <h5>This project is built using</h5>
                        <ul class="list-unstyled">
                            <li><a href="#" target="_blank"></a></li>
                            <li><a href="#" target="_blank"></a></li>
                            <li><a href="#" target="_blank"></a></li>
                            <li><a href="#" target="_blank"></a></li>
                        </ul>
                    </div>
                </div>
            </div>

            <!-- Copyright -->
            <div class="text-center pt-3">
                &copy; 2020-2022 Copyright <a href="#" target="_blank">Olaf Neumann</a>
            </div>
        </footer>
    </div>
</div>
