var appMessageQueue = [];

var options = {
	appMessage: {
		maxTries: 3,
		retryTimeout: 3000,
		timeout: 100,
		packetLength: 96
	},
	http: {
		timeout: 20000
	}
};

var List = {
	Book: 0,
	Viewer: 1
};

// bible structure
var bible = [];
bible.push([{"name":"Genesis","chapters":50},{"name":"Exodus","chapters":40},{"name":"Leviticus","chapters":27},{"name":"Numbers","chapters":36},{"name":"Deuteronomy","chapters":34},{"name":"Joshua","chapters":24},{"name":"Judges","chapters":21},{"name":"Ruth","chapters":4},{"name":"1 Samuel","chapters":31},{"name":"2 Samuel","chapters":24},{"name":"1 Kings","chapters":22},{"name":"2 Kings","chapters":25},{"name":"1 Chronicles","chapters":29},{"name":"2 Chronicles","chapters":36},{"name":"Ezra","chapters":10},{"name":"Nehemiah","chapters":13},{"name":"Esther","chapters":10},{"name":"Job","chapters":42},{"name":"Psalms","chapters":150},{"name":"Proverbs","chapters":31},{"name":"Ecclesiastes","chapters":12},{"name":"Song of Solomon","chapters":8},{"name":"Isaiah","chapters":66},{"name":"Jeremiah","chapters":52},{"name":"Lamentations","chapters":5},{"name":"Ezekiel","chapters":48},{"name":"Daniel","chapters":12},{"name":"Hosea","chapters":14},{"name":"Joel","chapters":3},{"name":"Amos","chapters":9},{"name":"Obadiah","chapters":1},{"name":"Jonah","chapters":4},{"name":"Micah","chapters":7},{"name":"Nahum","chapters":3},{"name":"Habakkuk","chapters":3},{"name":"Zephaniah","chapters":3},{"name":"Haggai","chapters":2},{"name":"Zechariah","chapters":14},{"name":"Malachi","chapters":4}]);
bible.push([{"name":"Matthew","chapters":28},{"name":"Mark","chapters":16},{"name":"Luke","chapters":24},{"name":"John","chapters":21},{"name":"Acts","chapters":28},{"name":"Romans","chapters":16},{"name":"1 Corinthians","chapters":16},{"name":"2 Corinthians","chapters":13},{"name":"Galatians","chapters":6},{"name":"Ephesians","chapters":6},{"name":"Philippians","chapters":4},{"name":"Colossians","chapters":4},{"name":"1 Thessalonians","chapters":5},{"name":"2 Thessalonians","chapters":3},{"name":"1 Timothy","chapters":6},{"name":"2 Timothy","chapters":4},{"name":"Titus","chapters":3},{"name":"Philemon","chapters":1},{"name":"Hebrews","chapters":13},{"name":"James","chapters":5},{"name":"1 Peter","chapters":5},{"name":"2 Peter","chapters":3},{"name":"1 John","chapters":5},{"name":"2 John","chapters":1},{"name":"3 John","chapters":1},{"name":"Jude","chapters":1},{"name":"Revelation","chapters":22}]);

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

function sendTextForVerse(text) {
	var messageCount = Math.ceil(text.length / options.appMessage.packetLength);
	for (var i = 0; i < messageCount; i++)
	{
		appMessageQueue.push({'message': {
			'list': List.Viewer,
			'request': true,
			'content': cleanString(text.substring(i * options.appMessage.packetLength, (i+1) * options.appMessage.packetLength))
		}});
	}
	sendAppMessageQueue();
}

function requestVerseText(book, chapter) {
	var xhr = new XMLHttpRequest();
	var url = "http://labs.bible.org/api/?passage="+encodeURI(book + ' ' + chapter)+"&type=json";
	console.log("Fetching verse data from: " + url);
	xhr.open('GET', url);
	xhr.timeout = options.http.timeout;
	xhr.onload = function(e) {
		if (xhr.readyState == 4) {
			if (xhr.status == 200) {
				if (xhr.responseText) {
					res = JSON.parse(xhr.responseText);
					var verseText = "";
					for (var i in res)
					{
						verseText += res[i].verse + ")" + res[i].text + " ";
					}
					sendTextForVerse(verseText);
				} else {
					console.log('Invalid response received! ' + JSON.stringify(xhr));
				}
			} else {
				console.log('Request returned error code ' + xhr.status.toString());
			}
		}
		sendAppMessageQueue();
	};
	xhr.ontimeout = function() {
		console.log('HTTP request timed out');
		appMessageQueue.push({'message': {'error': 'Error: Request timed out!'}});
		sendAppMessageQueue();
	};
	xhr.onerror = function() {
		console.log('HTTP request returned error');
		appMessageQueue.push({'message': {'error': 'Error: Failed to connect!'}});
		sendAppMessageQueue();
	};
	xhr.send(null);
}

Pebble.addEventListener('ready', function(e) {
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
			requestVerseText(e.payload.book, e.payload.chapter);
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

function cleanString(dirtyString) {
	dirtyString = dirtyString.replace(/&#8211;/g, "-");
	return dirtyString.replace(/”|“/g, '"');
}