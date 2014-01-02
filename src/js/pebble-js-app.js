var appMessageQueue = [];

var options = {
	appMessage: {
		maxTries: 3,
		retryTimeout: 3000,
		timeout: 100
	},
	http: {
		timeout: 20000
	}
};

var List = {
	Book: 0,
	Viewer: 1
};

// temporary bible structure:
var bible = [];

// test data until we have configuration page
bible.push([{name: 'Genesis', chapters: 21, tmp: ["verse 1 text", "verse 2 tesxt"]}, {name: 'Exodus', chapters: 12, tmp: ["verse 1 text", "verse 2 tesxt"]}]);
bible.push([{name: 'Matthew', chapters: 21, tmp: ["verse 1 text", "verse 2 tesxt"]}, {name: 'Mark', chapters: 30, tmp: ["verse 1 text", "verse 2 tesxt"]}, {name: 'Luke', chapters: 21, tmp: ["verse 1 text", "verse 2 tesxt"]}]);

// function sendPlayerList() {
// 	for (var i = 0; i < players.length; i++) {
// 		appMessageQueue.push({'message': {
// 			'index': i,
// 			'request': true,
// 			'title': players[i].title,
// 			'status': players[i].subtitle,
// 			'player': players[i].player
// 		}});
// 	}
// 	sendAppMessageQueue();
// }

function sendAppMessageQueue() {
	if (appMessageQueue.length > 0) {
		currentAppMessage = appMessageQueue[0];
		currentAppMessage.numTries = currentAppMessage.numTries || 0;
		currentAppMessage.transactionId = currentAppMessage.transactionId || -1;
		if (currentAppMessage.numTries < options.appMessage.maxTries) {
			console.log('Sending AppMessage to Pebble: ' + JSON.stringify(currentAppMessage.message));
			Pebble.sendAppMessage(
				currentAppMessage.message,
				function(e) {
					appMessageQueue.shift();
					setTimeout(function() {
						sendAppMessageQueue();
					}, options.appMessage.timeout);
				}, function(e) {
					console.log('Failed sending AppMessage for transactionId:' + e.data.transactionId + '. Error: ' + e.data.error.message);
					appMessageQueue[0].transactionId = e.data.transactionId;
					appMessageQueue[0].numTries++;
					setTimeout(function() {
						sendAppMessageQueue();
					}, options.appMessage.retryTimeout);
				}
			);
		} else {
			console.log('Failed sending AppMessage for transactionId:' + currentAppMessage.transactionId + '. Bailing. ' + JSON.stringify(currentAppMessage.message));
		}
	}
}

function sendBooksForTestament(testament) {
	var books = bible[testament];
	for (var i = 0; i < books.length; i++) {
		appMessageQueue.push({'message': {
			'list': List.Book,
			'index': i,
			'request': true,
			'book': books[i].name,
			'chapter': books[i].chapters
		}});
	}
	sendAppMessageQueue();
}

function sendTextForBookAndChapter(book, chapter) {
	appMessageQueue.push({'message': {
		'list': List.Viewer,
		'request': true,
		'content': "Content returned by the javascript api now"
	}});
	sendAppMessageQueue();
}

// function getBooksForTestament(testament) {
// 	var xhr = new XMLHttpRequest();
// 	xhr.open('GET', 'http://' + players[index].server.host + '/requests/status.json?' + request, true, '', players[index].server.pass);
// 	xhr.timeout = options.http.timeout;
// 	xhr.onload = function(e) {
// 		if (xhr.readyState == 4) {
// 			if (xhr.status == 200) {
// 				if (xhr.responseText) {
// 					res    = JSON.parse(xhr.responseText);
// 					title  = res.information || players[index].title;
// 					title  = title.category || players[index].title;
// 					title  = title.meta || players[index].title;
// 					title  = title.filename || players[index].title;
// 					title  = title.substring(0,30);
// 					status = res.state ? res.state.charAt(0).toUpperCase()+res.state.slice(1) : 'Unknown';
// 					status = status.substring(0,30);
// 					volume = res.volume || 0;
// 					volume = (volume / 512) * 200;
// 					volume = (volume > 200) ? 200 : volume;
// 					volume = Math.round(volume);
// 					length = res.length || 0;
// 					seek   = res.time || 0;
// 					seek   = (seek / length) * 100;
// 					seek   = Math.round(seek);
// 					appMessageQueue.push({'message': {'player': mediaPlayer.VLC, 'title': title, 'status': status, 'volume': volume, 'seek': seek}});
// 				} else {
// 					console.log('Invalid response received! ' + JSON.stringify(xhr));
// 					appMessageQueue.push({'message': {'player': mediaPlayer.VLC, 'title': 'Error: Invalid response received!'}});
// 				}
// 			} else {
// 				console.log('Request returned error code ' + xhr.status.toString());
// 				appMessageQueue.push({'message': {'Error: ' + xhr.statusText}});
// 			}
// 		}
// 		sendAppMessageQueue();
// 	};
// 	xhr.ontimeout = function() {
// 		console.log('HTTP request timed out');
// 		appMessageQueue.push({'message': {'error': 'Error: Request timed out!'}});
// 		sendAppMessageQueue();
// 	};
// 	xhr.onerror = function() {
// 		console.log('HTTP request returned error');
// 		appMessageQueue.push({'message': {'error': 'Error: Failed to connect!'}});
// 		sendAppMessageQueue();
// 	};
// 	xhr.send(null);
// }

Pebble.addEventListener('ready', function(e) {
// 	sendPlayerList();
	console.log('JS application ready to go!');
});

Pebble.addEventListener('appmessage', function(e) {
	console.log('AppMessage received from Pebble: ' + JSON.stringify(e.payload));

	var request = e.payload.request || '';
	switch (request) {
		case 'books':
			sendBooksForTestament(e.payload.testament);
			break;
		case 'viewer':
			sendTextForBookAndChapter(e.payload.book, e.payload.chapter);
			break;
	}
});

// Pebble.addEventListener('showConfiguration', function(e) {
// 	var uri = '';
// 	console.log('[configuration] uri: ' + uri);
// 	Pebble.openURL(uri);
// });

// Pebble.addEventListener('webviewclosed', function(e) {
// 	if (e.response) {
// 		var options = JSON.parse(decodeURIComponent(e.response));
// 		console.log('[configuration] options received: ' + JSON.stringify(options));
// 		// do something. store configuration to localStore and call refresh.
// 	} else {
// 		console.log('[configuration] no options received');
// 	}
// });

function isset(i) {
	return (typeof i != 'undefined')
}
