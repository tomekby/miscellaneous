package pl.vot.tomekby.mathGame.domain

import pl.vot.tomekby.mathGame.DataCallback
import pl.vot.tomekby.mathGame.EmptyCallback

data class GameConfig(var results: Int)
data class GameInfoDTO(val expression: String, val correctResult: Long, val choices: ArrayList<Long>)
data class HighScoreDTO(val member: String, val time: Double, val dateFinished: String)
typealias HighScoreList = List<HighScoreDTO>

interface GameService {
    fun getHighScores(onSuccess: DataCallback<HighScoreList>, onFailure: EmptyCallback)
    fun getState(onSuccess: DataCallback<GameInfoDTO>, onFailure: EmptyCallback)
    fun saveScore(time: Double)
}
