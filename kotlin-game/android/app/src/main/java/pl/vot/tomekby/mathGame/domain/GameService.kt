package pl.vot.tomekby.mathGame.domain

import org.jetbrains.anko.doAsync
import org.jetbrains.anko.uiThread
import pl.vot.tomekby.mathGame.di
import pl.vot.tomekby.mathGame.domain.auth.Auth
import pl.vot.tomekby.mathGame.domain.auth.Unauthorized

data class GameInfoDTO(val expression: String, val correctResult: Long, val choices: ArrayList<Long>)
data class HighScoreDTO(val member: String, val time: Double, val dateFinished: String)

interface GameService {
    fun getHighScores(onSuccess: (List<HighScoreDTO>) -> Unit, onFailure: () -> Unit)
    fun getState(onSuccess: (GameInfoDTO) -> Unit, onFailure: () -> Unit)
    fun saveScore(time: Double)
}
