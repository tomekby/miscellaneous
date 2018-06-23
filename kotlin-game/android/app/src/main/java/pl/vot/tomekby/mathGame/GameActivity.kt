package pl.vot.tomekby.mathGame

import android.annotation.SuppressLint
import android.graphics.Typeface
import org.jetbrains.anko.*
import android.os.Bundle
import android.support.v7.app.AppCompatActivity
import android.view.Gravity
import android.widget.LinearLayout
import android.widget.TextView
import org.jetbrains.anko.sdk25.coroutines.onClick
import pl.vot.tomekby.mathGame.domain.GameInfoDTO
import pl.vot.tomekby.mathGame.domain.GameService
import java.util.*
import kotlin.concurrent.fixedRateTimer

class GameActivity : AppCompatActivity() {

    private lateinit var timerView: TextView
    private lateinit var timer: Timer
    private lateinit var resultsAdapter: GameResultsAdapter
    private var startTime = 0L
    private var correct = 0L

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val progress = progressDialog(
            message = "Pobieranie stanu gry, proszę czekaj...",
            title = "Wczytywanie..."
        ) { isIndeterminate = true }
        // Read game state & init game
        di<GameService>().getState(
            { state ->
                initGame(state)
                startTimer()
                progress.dismiss()
            },
            {
                toast("Wystąpił problem podczas wczytywania stanu")
                this@GameActivity.finish()
            }
        )
    }

    /**
     * Set up layout
     */
    private fun initGame(state: GameInfoDTO): LinearLayout {
        correct = state.correctResult
        resultsAdapter = GameResultsAdapter(state.choices) { value, _ ->
            processResultButton(value)
        }

        return verticalLayout {
            gravity = Gravity.CENTER_HORIZONTAL
            verticalPadding = dip(10)

            /**
             * Const elements
             */
            textView("Oblicz wartość wyrażenia:") {
                typeface = Typeface.DEFAULT_BOLD
                padding = dip(10)
            }.lparams(width = wrapContent)
            textView {
                text = "%s = ?".format(state.expression)
                padding = dip(10)
            }.lparams(width = wrapContent)

            /**
             * Result choices
             */
            listView {
                adapter = resultsAdapter
                divider = null
                dividerHeight = 0
            }.lparams(height = dip(300))

            /**
             * Timer
             */
            timerView = textView("0 s.") {
                padding = dip(20)
            }.lparams(width = wrapContent) { margin = dip(15) }

            /**
             * Additional actions
             */
            linearLayout {
                this.gravity = Gravity.CENTER_HORIZONTAL
                button("restart") {
                    onClick { this@GameActivity.recreate() }
                }.lparams(width = wrapContent) { margin = dip(15) }

                button("Wyloguj") {
                    onClick { this@GameActivity.finish() }
                }.lparams(width = wrapContent) { margin = dip(15) }
            }
        }
    }

    private fun processResultButton(value: Long) {
        if (value != correct) {
            // Invalid result, let user play
            toast("Błędny wynik, spróbuj ponownie")
            return
        }

        // User clicked correct result
        timer.cancel()
        // Show info about success
        val time = timeTaken()
        alert("Prawidłowy wynik, twój czas: %.3f s.".format(time), "Gratulacje!") {
            positiveButton("Wyloguj") {
                this@GameActivity.finish()
            }
            negativeButton("Wróć") {}
        }.show()
        setTimerText(time)
        // Save current state on remote
        di<GameService>().saveScore(time)
        // Disable result buttons
        resultsAdapter.disableButtons()
    }

    private fun startTimer() {
        // Game timer
        timer = fixedRateTimer(name = "game-timer", period = 50) {
            // Try to update game time, if it fails then this activity was stopped
            try {
                runOnUiThread { setTimerText(timeTaken()) }
            } catch (e: RuntimeException) {
                cancel()
            }
        }
        // Set start time after everything was initialized
        startTime = System.currentTimeMillis()
    }

    private fun timeTaken(): Double {
        return (System.currentTimeMillis() - startTime).toDouble() / 1000
    }

    @SuppressLint("SetTextI18n")
    private fun setTimerText(time: Double) {
        timerView.text = "Czas gry: %.3f s.".format(time)
    }

}