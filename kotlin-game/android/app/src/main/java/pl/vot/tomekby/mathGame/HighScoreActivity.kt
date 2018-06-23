package pl.vot.tomekby.mathGame

import android.support.v7.app.AppCompatActivity
import android.os.Bundle
import android.view.Gravity
import org.jetbrains.anko.*
import org.jetbrains.anko.sdk25.coroutines.onClick
import pl.vot.tomekby.mathGame.domain.GameService

class HighScoreActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val highScoreAdapter = HighScoreAdapter()

        val progress = progressDialog(
            message = "Pobieranie wyników, proszę czekaj...",
            title = "Wczytywanie..."
        ) { isIndeterminate = true }
        // Fetch game state
        di<GameService>().getHighScores(
            // Fill high scores on success
            { items ->
                highScoreAdapter.add(items)
                progress.dismiss()
            },
            // On failure, just show info & go back
            {
                toast("Nie można pobrać wyników")
                finish()
            }
        )

        verticalLayout {
            padding = dip(10)
            textView {
                text = context.getString(R.string.bestScores)
                textSize = 30f
            }

            listView {
                adapter = highScoreAdapter
            }.lparams(height = dip(400))

            button {
                text = context.getString(R.string.backToLogin)
                onClick { _ -> this@HighScoreActivity.finish() }
            }.lparams {
                verticalMargin = dip(15)
                gravity = Gravity.CENTER_HORIZONTAL
            }
        }

    }
}
