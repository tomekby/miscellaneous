package pl.vot.tomekby.mathGame.domain.auth

import com.github.kittinunf.fuel.Fuel.Companion.post
import com.github.kittinunf.result.Result
import pl.vot.tomekby.mathGame.EmptyCallback

/**
 * Auth based on remote API (HTTP Basic Auth)
 */
class ApiAuth : Auth {

    override fun login(username: String, password: String, onSuccess: EmptyCallback, onFailure: EmptyCallback) {
        post("/login.php")
            .authenticate(username, password)
            .response { _, _, result ->
                if (result is Result.Success) {
                    saveAuthData(username, password)
                    onSuccess()
                } else onFailure()
            }
    }
}