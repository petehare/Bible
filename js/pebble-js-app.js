var appMessageQueues = {};

var DEBUG = true;
var logDebug = function() {
    if (DEBUG) {
        console.log.apply(console, arguments);
    }
};

var logError = function() {
    console.log.apply(console, arguments);
};

var options = {
	appMessage: {
		maxTries: 3,
		retryTimeout: 3000,
		timeout: 100,
		packetLength: 80,
        verseBatch: 15
	},
	http: {
		timeout: 20000
	}
};

var MessageType = {
	Book: 0,
    Verses: 1,
    Favorites: 2,
	Viewer: 3,
	FavoritesDidChange: 4,
	PebbleJSInitialized: 5
};

var Request = {
    Books: 0,
    Verses: 1,
    Viewer: 2,
    Cancel: 3,
    Favorites: 4,
    ToggleFavorite: 5
};

// Bible structure
var bible = [];
bible.push([{"name":"Genesis","chapters":50},{"name":"Exodus","chapters":40},{"name":"Leviticus","chapters":27},{"name":"Numbers","chapters":36},{"name":"Deuteronomy","chapters":34},{"name":"Joshua","chapters":24},{"name":"Judges","chapters":21},{"name":"Ruth","chapters":4},{"name":"1 Samuel","chapters":31},{"name":"2 Samuel","chapters":24},{"name":"1 Kings","chapters":22},{"name":"2 Kings","chapters":25},{"name":"1 Chronicles","chapters":29},{"name":"2 Chronicles","chapters":36},{"name":"Ezra","chapters":10},{"name":"Nehemiah","chapters":13},{"name":"Esther","chapters":10},{"name":"Job","chapters":42},{"name":"Psalms","chapters":150},{"name":"Proverbs","chapters":31},{"name":"Ecclesiastes","chapters":12},{"name":"Song of Solomon","chapters":8},{"name":"Isaiah","chapters":66},{"name":"Jeremiah","chapters":52},{"name":"Lamentations","chapters":5},{"name":"Ezekiel","chapters":48},{"name":"Daniel","chapters":12},{"name":"Hosea","chapters":14},{"name":"Joel","chapters":3},{"name":"Amos","chapters":9},{"name":"Obadiah","chapters":1},{"name":"Jonah","chapters":4},{"name":"Micah","chapters":7},{"name":"Nahum","chapters":3},{"name":"Habakkuk","chapters":3},{"name":"Zephaniah","chapters":3},{"name":"Haggai","chapters":2},{"name":"Zechariah","chapters":14},{"name":"Malachi","chapters":4}]);
bible.push([{"name":"Matthew","chapters":28},{"name":"Mark","chapters":16},{"name":"Luke","chapters":24},{"name":"John","chapters":21},{"name":"Acts","chapters":28},{"name":"Romans","chapters":16},{"name":"1 Corinthians","chapters":16},{"name":"2 Corinthians","chapters":13},{"name":"Galatians","chapters":6},{"name":"Ephesians","chapters":6},{"name":"Philippians","chapters":4},{"name":"Colossians","chapters":4},{"name":"1 Thessalonians","chapters":5},{"name":"2 Thessalonians","chapters":3},{"name":"1 Timothy","chapters":6},{"name":"2 Timothy","chapters":4},{"name":"Titus","chapters":3},{"name":"Philemon","chapters":1},{"name":"Hebrews","chapters":13},{"name":"James","chapters":5},{"name":"1 Peter","chapters":5},{"name":"2 Peter","chapters":3},{"name":"1 John","chapters":5},{"name":"2 John","chapters":1},{"name":"3 John","chapters":1},{"name":"Jude","chapters":1},{"name":"Revelation","chapters":22}]);

var bibleCache = {};

var favoriteList = new FavoriteList();

function sendAppMessageQueue(token) {
	if (typeof appMessageQueues[token] != 'undefined' && appMessageQueues[token].length > 0) {
		currentAppMessage = appMessageQueues[token][0];
		currentAppMessage.numTries = currentAppMessage.numTries || 0;
		currentAppMessage.transactionId = currentAppMessage.transactionId || -1;
		if (currentAppMessage.numTries < options.appMessage.maxTries) {
			logDebug('Sending AppMessage to Pebble: ' + JSON.stringify(currentAppMessage.message) + ', tries: ' + currentAppMessage.numTries);
			Pebble.sendAppMessage(
				currentAppMessage.message,
				function(e) {
				    if (typeof appMessageQueues[token] === 'undefined') {
				        return;
				    }	
					appMessageQueues[token].shift();
					setTimeout(function() {
						sendAppMessageQueue(token);
					}, options.appMessage.timeout);
				}, function(e) {
					logError('ERROR: Failed sending AppMessage', e);
					appMessageQueues[token][0].transactionId = e.data.transactionId;
					appMessageQueues[token][0].numTries++;
					setTimeout(function() {
						sendAppMessageQueue(token);
					}, options.appMessage.retryTimeout);
				}
			);
		} else {
			logError('ERROR: Failed sending AppMessage for transactionId:' + currentAppMessage.transactionId + '. Bailing. ' + JSON.stringify(currentAppMessage.message));
		}
	}
	else
	{
		delete appMessageQueues[token];
	}
}

function cancelAppMessageQueue(token) {
	appMessageQueues[token.toString()] = [];
}

function sendBooksForTestament(testament, token) {
	var books = bible[testament];
	appMessageQueues[token.toString()] = [];
	for (var i = 0; i < books.length; i++) {
		appMessageQueues[token.toString()].push({'message': {
			'token': token,
			'messageType': MessageType.Book,
			'index': i,
			'book': books[i].name,
			'chapter': books[i].chapters
		}});
	}
	sendAppMessageQueue(token.toString());
}

// API requests

function requestVerseRanges(book, chapter, token) {
  getVerseText(token, book, chapter, function(response) {
    var batches = Math.ceil(response.length / options.appMessage.verseBatch);
    appMessageQueues[token.toString()] = [];
    for (var i = 0; i < batches; i++)
    {
     var batchName = ((i * options.appMessage.verseBatch) + 1).toString() + "-" + (Math.min((i + 1) * options.appMessage.verseBatch, response.length)).toString();
      appMessageQueues[token.toString()].push({'message': {
        'token': token,
        'messageType': MessageType.Verses,
        'index': i,
        'content': batchName
      }});
    }
    sendAppMessageQueue(token.toString());
  });
}

function requestFavorites(token) {
    appMessageQueues[token.toString()] = [];
    if (favoriteList.count() > 0) {
        for (var i = 0; i < favoriteList.count(); i++) {
            var favorite = favoriteList.favoriteAtIndex(i);
            appMessageQueues[token.toString()].push({'message': {
                'token': token,
                'messageType': MessageType.Favorites,
                'index': i,
                'book': favorite.book,
                'chapter': favorite.chapter,
                'range': favorite.range,
            }});
        }
    }
    else {
        appMessageQueues[token.toString()].push({'message': {
            'token': token,
            'messageType': MessageType.Favorites
        }});
    }
    sendAppMessageQueue(token.toString());
}

function requestVerseText(book, chapter, rangeString, token) {
  var range = rangeString.split("-");
  getVerseText(token, book, chapter, function(response) {
    var verseText = "";
    for (var i in response)
    {
      if (parseInt(response[i].verse) >= parseInt(range[0]) && parseInt(response[i].verse) <= parseInt(range[1]))
      {
        verseText += response[i].verse + ") " + response[i].text + " ";
      }
    }

    text = cleanString(verseText);
    var messageCount = Math.ceil(text.length / options.appMessage.packetLength);
    appMessageQueues[token.toString()] = [];
    for (var j = 0; j < messageCount; j++)
    {
      appMessageQueues[token.toString()].push({'message': {
        'token': token,
        'messageType': MessageType.Viewer,
        'index': j,
        'content': text.substring(j * options.appMessage.packetLength, (j+1) * options.appMessage.packetLength)
      }});
    }
    sendAppMessageQueue(token.toString());
  });
}

function toggleFavorite(book, chapter, range, token) {
    
    var favorite = new Favorite({book: book, chapter: chapter, range: range});
    var didChange = false;
    
    if (favoriteList.contains(favorite)) {
        didChange = favoriteList.remove(favorite);
        Pebble.showSimpleNotificationOnPebble("Favorite Removed", book + " " + chapter + ":" + range + " was removed from your favorites");
    } else {
        didChange = favoriteList.add(favorite);
        Pebble.showSimpleNotificationOnPebble("Favorite Added", book + " " + chapter + ":" + range + " was added to your favorites");
    }
    
    if (didChange) {
        appMessageQueues[token.toString()] = [];
        appMessageQueues[token.toString()].push({'message': {
            'token': token,
            'messageType': MessageType.FavoritesDidChange
        }});
        sendAppMessageQueue(token.toString());
    }
}

function getVerseText(token, book, chapter, completion) {

    if (bibleCache.hasOwnProperty(book+chapter))
    {
        completion(bibleCache[book+chapter]);
        return;
    }

	var xhr = new XMLHttpRequest();
	var url = "http://labs.bible.org/api/?passage="+encodeURI(book + ' ' + chapter)+"&type=json";
	logDebug("Fetching verse data from: " + url);
	xhr.open('GET', url);
	xhr.timeout = options.http.timeout;
	xhr.onload = function(e) {
		if (xhr.readyState == 4) {
			if (xhr.status == 200) {
				if (xhr.responseText) {
					var res = JSON.parse(xhr.responseText);
                    bibleCache[book+chapter] = res;
                    completion(res);
				} else {
					logError('ERROR: Invalid response received! ' + JSON.stringify(xhr));
				}
			} else {
				logError('ERROR: Request returned error code ' + xhr.status.toString());
			}
		}
	};
	xhr.ontimeout = function() {
		logError('ERROR: HTTP request timed out');
		appMessageQueues[token.toString()].push({'message': {'error': 'Error: Request timed out!'}});
		sendAppMessageQueue(token.toString());
	};
	xhr.onerror = function() {
		logError('ERROR: HTTP request returned error');
		appMessageQueues[token.toString()].push({'message': {'error': 'Error: Failed to connect!'}});
		sendAppMessageQueue(token.toString());
	};
	xhr.send(null);
}

Pebble.addEventListener('ready', function(e) {
	logDebug('JS application ready to go!');
    setTimeout(function() {
        Pebble.sendAppMessage({
                'messageType': MessageType.PebbleJSInitialized
            },
            function(e) {	
            }, 
            function(e) {
                logError('ERROR: Failed sending Initialization Message', e);
            }
        );
	}, 10);
});

Pebble.addEventListener('appmessage', function(e) {
	logDebug('AppMessage received from Pebble: ' + JSON.stringify(e.payload));

	var request = e.payload.request;
	var token = e.payload.token || 0;
	switch (request) {
		case Request.Books:
			sendBooksForTestament(e.payload.testament, token);
			break;
        case Request.Verses:
            requestVerseRanges(e.payload.book, e.payload.chapter, token);
            break;
		case Request.Viewer:
			requestVerseText(e.payload.book, e.payload.chapter, e.payload.range, token);
			break;
		case Request.Cancel:
			cancelAppMessageQueue(token);
			break;
        case Request.Favorites:
            requestFavorites(token);
            break;
        case Request.ToggleFavorite:
            toggleFavorite(e.payload.book, e.payload.chapter, e.payload.range, token);
            break;
	}
});

function cleanString(dirtyString) {
	dirtyString = dirtyString.replace(/<b>|<\/b>/g, "");
	dirtyString = dirtyString.replace(/\&#8211;/g, "-");
	dirtyString = dirtyString.replace(/‘|’/g, "'");
	return dirtyString.replace(/”|“/g, '"');
}