package pl.vot.tomekby.mathGame.domain

import android.annotation.SuppressLint
import android.database.sqlite.SQLiteException
import org.jetbrains.anko.db.SqlOrderDirection.*
import org.jetbrains.anko.db.classParser
import org.jetbrains.anko.db.insert
import org.jetbrains.anko.db.select
import org.jetbrains.anko.doAsync
import org.jetbrains.anko.uiThread
import pl.vot.tomekby.mathGame.di
import pl.vot.tomekby.mathGame.domain.SQLite.Companion.HIGH_SCORES_TABLE
import pl.vot.tomekby.mathGame.domain.auth.Auth
import pl.vot.tomekby.mathGame.domain.auth.Unauthorized
import java.text.SimpleDateFormat
import java.util.*
import java.util.concurrent.ThreadLocalRandom

fun ClosedRange<Int>.random() = ThreadLocalRandom.current().nextInt(endInclusive + 1 - start) + start
fun <T> List<T>.random(): T = this[ThreadLocalRandom.current().nextInt(size)]
fun <T> ArrayList<T>.setRandom(item: T) { this[ThreadLocalRandom.current().nextInt(size)] = item }
@SuppressLint("SimpleDateFormat")
fun Date.dbFormat(): String = SimpleDateFormat("dd.MM.yyyy").format(this)

class LocalGameService : GameService {
    companion object {
        // expression constraints
        private val operandRange = 1..10
        private val operators = listOf("+", "-", "*")
        private val resultsRange = 0..3
        // maximum relative deviation from result; in range 0 (equals) to 1 (result value away)
        private const val resultsVariation = 0.2
    }

    override fun getHighScores(onSuccess: (List<HighScoreDTO>) -> Unit, onFailure: () -> Unit) {
        doAsync {
            try {
                val scores = di<SQLite>().use {
                     select(HIGH_SCORES_TABLE)
                        .orderBy("time", ASC)
                        .parseList(classParser<HighScoreDTO>())
                }
                uiThread { onSuccess(scores) }
            } catch (e: SQLiteException) {
                uiThread { onFailure() }
            }
        }
    }

    override fun getState(onSuccess: (GameInfoDTO) -> Unit, onFailure: () -> Unit) {
        val op1 = operandRange.random().toLong()
        val op2 = operandRange.random().toLong()
        val operator = operators.random()
        val result = when (operator) {
            "+" -> op1 + op2
            "-" -> op1 - op2
            "*" -> op1 * op2
            else -> 0L
        }
        // Get random choices
        val choices = ArrayList(resultsRange.map {
            getResultValuesRange(result).random().toLong()
        })
        choices.setRandom(result)

        onSuccess(
            GameInfoDTO(
                expression = "%d %s %d".format(op1, operator, op2),
                correctResult = result,
                choices = choices
            )
        )
    }

    private fun getResultValuesRange(result: Long): IntRange {
        val start = (result * (1 - resultsVariation)).toInt()
        val end = (result * (1 + resultsVariation)).toInt()
        return start..end
    }

    @SuppressLint("SimpleDateFormat")
    override fun saveScore(time: Double) {
        doAsync {
            di<SQLite>().use {
                insert(HIGH_SCORES_TABLE,
                    "member" to Auth.username,
                    "time" to time,
                    "dateFinished" to Date().dbFormat()
                )
            }
        }
    }
}