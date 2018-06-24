package pl.vot.tomekby.mathGame.domain.auth

import org.jetbrains.anko.doAsync
import org.jetbrains.anko.uiThread
import pl.vot.tomekby.mathGame.EmptyCallback

/**
 * Auth based on hardcoded data
 */
class DummyAuth : Auth {
    companion object {
        // Auth data
        private const val validLogin = "wsb"
        private const val validPassword = "wsb"
    }

    override fun login(username: String, password: String, onSuccess: EmptyCallback, onFailure: EmptyCallback) {
        doAsync {
            if (username == validLogin && password == validPassword) {
                saveAuthData(username, password)
                uiThread { onSuccess() }
            } else {
                uiThread { onFailure() }
            }
        }
    }
}