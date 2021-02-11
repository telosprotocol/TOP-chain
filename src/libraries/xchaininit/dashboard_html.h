#pragma once

#include <string>

const std::string admin_dashboard_html = R"(
<html>
    <head>
        <title>welcom to topio-admin</title>
        <style type="text/css">
            a[href^="#"] {
                text-decoration: none;
                font-family: monospace
            }

            .tip {
                font-size: 0.85em
            }

            a:hover {
                background-color: #000;
                color: #fff
            }

            div.nopad {
                padding: 0px
            }

            #header {
                margin: auto;
                margin-top: 1em;
                padding: 0em;
                padding-top: 0.25em;
                padding-bottom: 0.25em;
                border: 5px solid #595540;
                text-align: center;
                width: 600px;
                background-color: #cc8139
            }

            #log {
                width: 90%;
                height: 85%;
                background-color: #ffefd5;
                margin: auto;
                margin-top: -5px;
                padding: 1em 1em 1em 1em;
                border: 5px solid #595540;
                overflow: auto;
                font-family: monospace;
                color: #262626;
                font-size: 1.25em;
                word-wrap: break-word
            }

            #log h1 {
                font-size: 1.5em;
                line-height: 1.25em
            }

            #log .line {
                margin: 0em 0em 1em 0em;
                line-height: 1.25em
            }

            #log span.prompt {
                font-weight: normal;
                color: #595540
            }

            #log .input {
                font-weight: bold;
                color: #595540
            }

            #log .input a {
                color: #595540
            }

            #log .response {
                font-weight: bold;
                color: #262626
            }

            #log .notification {
                font-weight: normal;
                background-color: #262626;
                color: #cc8139;
                font-family: sans-serif;
                font-size: 0.75em;
                padding: 1em
            }

            #log .error {
                font-weight: normal;
                background-color: #8b0000;
                color: #fff;
                font-family: sans-serif;
                font-size: 0.75em;
                padding: 1em
            }

            #log a {
                color: #ffdab9
            }

            #log a.comment {
                color: gray
            }

            #log .tutorial_next {
                margin: 0px;
                text-align: right
            }

            #log pre {
                margin-left: 2em
            }

            #log li {
                padding: 0.25em
            }

            #toolbar {
                width: 750px;
                margin: auto;
                padding: 0em;
                border: 0px;
                margin-top: -5px;
            }

            #toolbar #input {
                padding: 0.5em;
                width: 100%;
                font-size: 1.5em;
                background-color: #cc8139;
                color: #262626;
                font-family: monospace;
                border: 5px solid #595540;
                outline: none;
                background-image: url(/images/prompt.png);
                background-repeat: no-repeat;
                padding-left: 45px
            }

            #footer {
                width: 600px;
                margin: auto;
                text-align: center;
                color: #ffefd5;
                font-size: 0.75em
            }

            #footer a {
                color: #ffdab9
            }

            body {
                background-color: #262626
            }
        </style>
        <script src="https://cdnjs.cloudflare.com/ajax/libs/jquery/3.5.1/jquery.min.js"></script>
    </head>
    <body>
        <div id='log'>
            <div class='line notification'>
                <p>
                    Welcome to <strong>topio-admin</strong>
                    , admin dashbord of topio.
		
                </p>
                <p>you can input command here to query info about topio
		</p>
                <pre>
                    <p>NAME:
    web-console
USAGE:
    command [arguments...]
COMMANDS:
    help                     Show a list of commands and options.
    topcl                    A command line interface to interact with the blockchain and manage accounts.
    xnode                    Xnode is the core service daemon that runs on every TOP Network node.
			</p>
                </pre>
            </div>
        </div>
        <div id='toolbar'>
            <input id='input' spellcheck='false'>
        </div>
        <div id='footer'>
            <p>This site is admin dashbord of topio, you can query info and status of topio,
        and you can also manage and monitor topio.
	</p>
        </div>
        <script type='text/javascript'>
            String.format = function() {
                var param = [];
                for (var i = 0, l = arguments.length; i < l; i++) {
                    param.push(arguments[i]);
                }
                var statment = param[0];
                // get the first element(the original statement)
                param.shift();
                // remove the first element from array
                return statment.replace(/\{(\d+)\}/g, function(m, n) {
                    return param[n];
                });
            }
            function ajax_topio_query(cmdline, module_command, sub_module_cmd) {
                $.ajax({
                    cache: false,
                    type: "POST",
                    url: "/api/" + module_command,
                    data: JSON.stringify({
                        "command": sub_module_cmd
                    }),
                    contentType: "application/json",
                    dataType: "json",
                    async: false,
                    error: function(request) {
                        alert("send request failed, please make sure remote http-server is ok.");
                    },
                    success: function(response) {
                        var result = '';
                        var status_code = response['status'];
                        if (status_code != 'ok') {
                            var error = response['error'];
                            result = error;
                        } else {
                            var data = response['data'];
                            for (var index in data) {
                                result = data[index];
                                break;
                            }
                        }
                        var cmd_line_div = document.createElement("div");
                        cmd_line_div.className = "line input";
                        cmd_line_div.innerHTML = String.format('<div class="nopad"><span class="prompt">&gt; </span><a href="#run">{0}</a></div>', cmdline);
                        var cmd_result_div = document.createElement("div");
                        cmd_result_div.className = "line response";
                        cmd_result_div.innerHTML = String.format('<div class="nopad"><span class="prompt"></span><pre><p>{0}</p></pre></div>', result);
                        var output_father = document.getElementById('log');
                        output_father.appendChild(cmd_line_div);
                        output_father.appendChild(cmd_result_div);

                        output_father.scrollTop = output_father.scrollHeight;
                    }
                });
            }

            $(function() {

                $('input').bind('keypress', function(event) {

                    if (event.keyCode == "13") {

                        var cmdline = document.getElementById("input").value;
                        if (cmdline == '') {
                            return;
                        }
                        var sp_command = cmdline.split(' ');
                        if (sp_command.length < 1) {
                            alert("wrong command, try input help");
                            return;
                        }
                        var module_command = sp_command[0];
                        if (module_command != 'xnode' && module_command != 'topcl') {
                            alert("wrong command, try input help");
                            return;
                        }
                        if (cmdline.indexOf('wallet') != -1) {
                            alert("not support topcl wallet command in dashbord, please use in attach mode");
                            return;
                        }

                        var body = new Object();
                        var sub_module_cmd = cmdline.substr(module_command.length, cmdline.length - module_command.length);

                        ajax_topio_query(cmdline, module_command, sub_module_cmd);
                        document.getElementById("input").value = '';
                    }

                });

            });
        </script>
    </body>
</html>

)";
