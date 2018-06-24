package pl.vot.tomekby.mathGame.domain.auth

import pl.vot.tomekby.mathGame.EmptyCallback

interface Auth {
    companion object {
        lateinit var username: String
            private set
        lateinit var password: String
            private set
    }

    fun login(username: String, password: String, onSuccess: EmptyCallback, onFailure: EmptyCallback)
    fun saveAuthData(_username: String, _password: String) {
        username = _username
        password = _password
    }
}