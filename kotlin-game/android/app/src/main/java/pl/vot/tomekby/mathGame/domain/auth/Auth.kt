package pl.vot.tomekby.mathGame.domain.auth

class Unauthorized(override var message:String): Exception(message)

interface Auth {
    companion object {
        lateinit var username: String
            private set
        lateinit var password: String
            private set
    }

    fun login(username: String, password: String, onSuccess: () -> Unit, onFailure: () -> Unit)
    fun saveAuthData(_username: String, _password: String) {
        username = _username
        password = _password
    }
}