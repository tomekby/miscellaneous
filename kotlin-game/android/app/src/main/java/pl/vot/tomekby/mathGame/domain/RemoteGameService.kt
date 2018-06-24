package pl.vot.tomekby.mathGame.domain

import com.fasterxml.jackson.module.kotlin.jacksonObjectMapper
import com.fasterxml.jackson.module.kotlin.readValue
import com.github.kittinunf.fuel.Fuel.Companion.get
import com.github.kittinunf.fuel.Fuel.Companion.post
import com.github.kittinunf.result.Result
import pl.vot.tomekby.mathGame.DataCallback
import pl.vot.tomekby.mathGame.EmptyCallback
import pl.vot.tomekby.mathGame.di
import pl.vot.tomekby.mathGame.domain.auth.Auth.Companion.password
import pl.vot.tomekby.mathGame.domain.auth.Auth.Companion.username

/**
 * Class processing remote game state
 * All requests except high scores have to be authenticated using HTTP basic auth
 */
class RemoteGameService : GameService {

    override fun getHighScores(onSuccess: DataCallback<HighScoreList>, onFailure: EmptyCallback) {
        get("/scores.json")
            .responseString {_, _, result ->
                if (result !is Result.Success) {
                    onFailure()
                    return@responseString
                }
                // Request was fine, so sort results & pass them to success handler
                val items = jacksonObjectMapper()
                    .readValue<List<HighScoreDTO>>(result.get())
                    .sortedWith(compareBy(HighScoreDTO::time))
                onSuccess(items)
            }
    }

    // Fetch game state & call proper callback on finish
    override fun getState(onSuccess: DataCallback<GameInfoDTO>, onFailure: EmptyCallback) {
        get("/", listOf("choices" to di<GameConfig>().results))
            .authenticate(username, password)
            .responseString { _, _, result ->
                if (result !is Result.Success) {
                    onFailure()
                    return@responseString
                }
                // Call success handler with deserialized response
                onSuccess(
                    jacksonObjectMapper()
                        .readValue(result.get())
                )
            }
    }

    override fun saveScore(time: Double) {
        post("save.php", listOf("time" to time))
            .authenticate(username, password)
            .response { _, _, _ -> }
    }
}