Pebble.addEventListener('ready', function() {
	console.log('PebbleKit JS ready!');
});

Pebble.addEventListener('showConfiguration', function() {
	var url = 'http://armstrongwill.com/pebbleconfigurationpages/centerhours/index.html';

	console.log('Showing configuration page: ' + url);

	Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(e) {
	var configData = JSON.parse(decodeURIComponent(e.response));

	console.log('Configuration page returned: ' + JSON.stringify(configData));

	if (configData.backgroundColor) {
		Pebble.sendAppMessage({
			backgroundColor : parseInt(configData.backgroundColor, 16),
			minutesColor : parseInt(configData.minutesColor, 16),
			twentyFourHourFormat : configData.twentyFourHourFormat
		}, function() {
			console.log("send successful!");
		}, function() {
			console.log("send failed!");
		});
	}
});
