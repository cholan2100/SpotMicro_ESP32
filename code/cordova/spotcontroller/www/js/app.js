var app = {
    user: {},
    debug: false,
    icon: "/android_asset/www/spote32.jpg",
    initialize: function () {
        console.log('app initialize');
        document.addEventListener('deviceready', this.onDeviceReady.bind(this), false);
    },

    onDeviceReady: function () {
        debug.log('device ready', 'success');
        screen.orientation.lock('landscape');
        app.bindEvents();

        controller.initialize();
        $('#controls').hide();
        $('#spotlink').hide(); 
        $('#sleep').hide();
        $('#spotlink').show(); 
        
        bluetooth.initialize();

        $('#device').on('click', '#refreshDeviceList', function (e) {
            bluetooth.refreshDeviceList();
        });

        var previousConnectedDevice = storage.getItem('connectedDevice');
        console.log("previousConnectedDevice " + previousConnectedDevice);

        if (previousConnectedDevice != undefined)
        {
            var html = '<ons-list-item id="' + previousConnectedDevice.id +
                        '" data-device-id="' + previousConnectedDevice.id + 
                        '" data-device-name="' + previousConnectedDevice.name + '" tappable>' +
                '<div class="left"><img class="list-item__thumbnail" src="'+ app.icon +'"></div>' +
                '<div class="center">' +
                    '<span class="list-item__title">' + previousConnectedDevice.name + '</span>' +
                    '<span class="list-item__subtitle">' + previousConnectedDevice.id + '</span>' +
                '</div>' +
                '<div class="right"><ons-progress-circular indeterminate></ons-progress-circular></div>' +
                '</ons-list-item>';
            $('#spot-devices-div').append(html);
            
        } else {
            // storage.removeItem('connectedDevice');
            // $('#spotlink').show();
        }
        $('#spotlink').show(); 

        // voice recognition
        $("#voice_but").hide();
        window.plugins.speechRecognition.isRecognitionAvailable(function(available){
            if(available) {
                window.plugins.speechRecognition.hasPermission(function (isGranted){
                    if(isGranted){
                        $("#voice_but").show();
                    }else{
                        window.plugins.speechRecognition.requestPermission(function (){
                            $("#voice_but").show();
                        }, function (err){
                            voice_ready = false;
                        });
                    }
                }, function(err){
                    console.log(err);
                });
            }
        }, function(err){
            console.error(err);
        });

        voice_but1 = document.getElementById('voice_but1');
        voice_but1.onclick = app.voice_recog;
        voice_but2 = document.getElementById('voice_but2');
        voice_but2.onclick = app.voice_recog;
    },

    voice_recog: function () {
        var settings = {
            lang: "en-US",
            showPopup: true
        };
        
        window.plugins.speechRecognition.startListening(function(result){
            debug.log(result);
            const artyom = new Artyom();
            artyom.addCommands([
                {
                    indexes: ["Hello spot wakeup","Hello spot wake up", "wake up"],
                    action: function(){
                        console.log("VOICE: Waking up spot...");
                        controller.wakeup();
                    }
                },
                {
                    indexes: ["Hello spot sleep","sleep"],
                    action: function(){
                        console.log("VOICE: Putting to sleep...");
                        controller.sleep_btn();
                    }
                }
            ]);

            result.forEach(function(option){
                artyom.simulateInstruction(option)
            });

        }, function(err){
            console.log(err);
        }, settings);
    },

    bindEvents: function () {
        // setTimeout(function () {
        //     mqttclient.addMessage('app,1');
        // }, 3000);

        document.addEventListener("pause", app.onDevicePause, false);
        document.addEventListener("resume", app.onDeviceResume, false);
        document.addEventListener("menubutton", app.onMenuKeyDown, false);

        $('#spot-devices-div').on('click', 'ons-list-item', function (e) {
            if($(this).attr("data-device-conn") == "idle" || $(this).attr("data-device-conn") == "error")
            {
                var html = '<ons-list-item id="' + $(this).attr("data-device-id") +
                             '" data-device-id="' + $(this).attr("data-device-id") +
                             '" data-device-name="' + $(this).attr("data-device-name") +
                              '" data-device-conn="connecting" tappable>' +
                            '<div class="left"><img class="list-item__thumbnail" src="'+ app.icon +'"></div>' +
                            '<div class="center">' +
                                '<span class="list-item__title">' + $(this).attr("data-device-name") + '</span>' +
                                '<span class="list-item__subtitle">' + $(this).attr("data-device-id") + '</span>' +
                            '</div>' +
                            '<div class="right"><ons-progress-circular indeterminate></ons-progress-circular></div>' +
                            '</ons-list-item>';
                
                    $(this).remove();
                    $('#spot-devices-div').append(html);

                    bluetooth.connectDevice($(this).attr("data-device-id"), $(this).attr("data-device-name"));
            }
        });

        document.getElementById('ble_button').onclick = function() {
            storage.removeItem('connectedDevice');
            bluetooth.disconnectDevice(false);
        }
        document.getElementById('sleep_btn').onclick = function() {
            bluetooth.sendOrientation(0, 0, 0, -40, -170, 0);
        }
        document.getElementById('wakeup_btn').onclick = function() {
            bluetooth.sendOrientation(0, 0, 0, 0, 0, 0);
        }
    },

    onDevicePause: function () {
        debug.log('in pause');
    },
    onDeviceResume: function () {
        debug.log('out of pause');
        bluetooth.refreshDeviceList();
    },
    onMenuKeyDown: function () {
        debug.log('menubuttonpressed');
    },
    onError: function (error) {
        debug.log(JSON.stringify(error), 'error');
    }
};

app.initialize();