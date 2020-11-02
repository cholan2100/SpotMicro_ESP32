var app = {
    user: {},
    debug: false,
    currentView: 0, // 0: controls, 1: ble
    initialize: function () {
        console.log('app initialize');
        document.addEventListener('deviceready', this.onDeviceReady.bind(this), false);
    },

    onDeviceReady: function () {
        debug.log('device ready', 'success');
        app.bindEvents();
        bluetooth.initialize();
        controller.initialize();

        bluetooth.toggleConnectionButtons();
        screen.orientation.lock('landscape');

        $('#headerbar').on('click', '#ble_button', function (e) {
            if (app.currentView == 0) {
                $('#device').show();
                $('#controls').hide();
                app.currentView  = 1;
            } else {
                $('#device').hide();
                $('#controls').show();
                app.currentView  = 0;
            }
        });

        $('#device').on('click', '#refreshDeviceList', function (e) {
                bluetooth.refreshDeviceList();
        });
        $('#ble-found-devices').on('click', 'ons-list-item', function (e) {
                bluetooth.connectDevice($(this).attr("data-device-id"), $(this).attr("data-device-name"));
        });
        $('#device').on('click', '#disconnectDevice', function (e) {
                bluetooth.disconnectDevice(e);
        });


        // wait for the spot to connect
        $('#headerbar').hide();
        $('#controls').hide();
        $('#spotlink').hide(); 
        $('#device').hide();

        var previousConnectedDevice = storage.getItem('connectedDevice');
        console.log("previousConnectedDevice " + previousConnectedDevice);

        if (previousConnectedDevice != undefined)
        {
            let spotImage = "/android_asset/www/cesar.jpg";
            var html = '<ons-list-item id="' + previousConnectedDevice.id +
                        '" data-device-id="' + previousConnectedDevice.id + 
                        '" data-device-name="' + previousConnectedDevice.name + '" tappable>' +
                '<div class="left"><img class="list-item__thumbnail" src="'+ spotImage +'"></div>' +
                '<div class="center">' +
                    '<span class="list-item__title">' + previousConnectedDevice.name + '</span>' +
                    '<span class="list-item__subtitle">' + previousConnectedDevice.id + '</span>' +
                '</div>' +
                '<div class="right"><ons-progress-circular indeterminate></ons-progress-circular></div>' +
                '</ons-list-item>';
            $('#spot-devices-div').append(html);
            $('#spotlink').show(); 
        } else {
            // storage.removeItem('connectedDevice');
            $('#spotlink').show();
        }

        $('#spot-devices-div').on('click', 'ons-list-item', function (e) {
            if($(this).attr("data-device-conn") == "idle" || $(this).attr("data-device-conn") == "error")
            {
                let spotImage = "/android_asset/www/cesar.jpg";
                var html = '<ons-list-item id="' + $(this).attr("data-device-id") +
                             '" data-device-id="' + $(this).attr("data-device-id") +
                             '" data-device-name="' + $(this).attr("data-device-name") +
                              '" data-device-conn="connecting" tappable>' +
                            '<div class="left"><img class="list-item__thumbnail" src="'+ spotImage +'"></div>' +
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
            console.log(result);
            // By default just 5 options
            // ["Hello","Hallou", "Hellou" ...]
            // import Artyom from "artyom.js"
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
                },
                {
                    indexes: ["Translate * in Spanish"],
                    smart: true,
                    action: function(i, wildcard){
                        console.log("I cannot translate" + wildcard);
                    }
                },
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