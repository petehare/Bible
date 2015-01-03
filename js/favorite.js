/*
 * Favorite Bible verse
 * @param jsonObject An object with keys for book, chapter, and range
 * @return Returns the new favorite object
 */
function Favorite(jsonObject) {
    
    this.book = jsonObject.book;
    this.chapter = jsonObject.chapter;
    this.range = jsonObject.range;
    
    this.isEqual = function(aFavorite) {
        if (aFavorite.book != this.book) return false;
        if (aFavorite.chapter != this.chapter) return false;
        if (aFavorite.range != this.range) return false;
        return true;
    };
    
    this.asJSONObject = function() {
        return {book: this.book, chapter: this.chapter, range: this.range};
    };
    
}

function FavoriteList() {
    
    this.favorites = [];
    
    this.count = function() {
        return this.favorites.length;
    };
    
    this.favoriteAtIndex = function(index) {
        return this.favorites[index];
    };
    
    /*
     * Check if favorite exists
     * @param aFavorite Favorite object to test
     * @return Returns true if favorite exists
     */
    this.contains = function (aFavorite) {
        for (var i = 0; i < this.favorites.length; i++) {
            var favorite = this.favorites[i];
            if (favorite.isEqual(aFavorite)) {
                return true;
            }
        }
        return false;
    };
    
    /*
     * Add a new favorite
     * @param aFavorite Favorite object to add
     * @return Returns true if added or false if it already existed
     */
    this.add = function(aFavorite) {
        if (this.contains(aFavorite)) {
            return false;
        }
        this.favorites.push(aFavorite);
        return true;
    };

    /*
     * Remove a favorite
     * @param aFavorite Favorite object to be removed
     * @return Returns true if removed or false if it never existed
     */
    this.remove = function(aFavorite) {
        
        var favoriteIndex = -1;
        for (var i = 0; i < this.favorites.length; i++) {
            var favorite = this.favorites[i];
            if (favorite.isEqual(aFavorite)) {
                favoriteIndex = i;
                break;
            }
        }
        if (i > -1) {
            this.favorites.splice(favoriteIndex, 1);
        }
        return false;
    };
    
    /*
     * Saves the favorite list to persistent storage
     */
    this.save = function() {
        var jsonFavorites = [];
        for (var i = 0; i < this.favorites.length; i++) {
            var favorite = this.favorites[i];
            jsonFavorites.push(favorite.asJSONObject());
        }
        localStorage.setItem('favoriteList', JSON.stringify(jsonFavorites));
    };

    /*
     * Reloads the favorite list from persistent storage
     */
    this.reload = function() {
        var jsonFavorites = JSON.parse(localStorage.getItem('favoriteList')) || [];
        for (var i = 0; i < jsonFavorites.length; i++) {
            var favorite = new Favorite(jsonFavorites[i]);
            this.add(favorite);
        }
    };
    
    this.reload();
}
