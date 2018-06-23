package pl.vot.tomekby.mathGame

import android.support.v7.app.AppCompatActivity
import android.os.Bundle
import kotlinx.android.synthetic.main.activity_main.*
import org.jetbrains.anko.alert
import org.jetbrains.anko.sdk25.coroutines.onClick
import org.jetbrains.anko.startActivity
import org.jetbrains.anko.toast
import pl.vot.tomekby.mathGame.domain.auth.Auth

class MainActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        loginBtn.onClick { _ ->
            login(username.text.toString(), password.text.toString())
        }

        gotoHighscore.onClick { _ ->
            startActivity<HighScoreActivity>()
            username.setText("")
            password.setText("")
        }

        gameMode.onClick { _ ->
            alert(title = "Wybierz tryb gry", message = "Gra lokalna jest prostsza, ale wyniki są widoczne tylko dla ciebie") {
                positiveButton("Lokalnie") {
                    MainApplication.Dependency.local()
                }
                negativeButton("Przez internet") {
                    MainApplication.Dependency.remote()
                }
            }.show()
        }
    }

    private fun login(username: String, password: String) {
        if (username == "" || password == "") {
            toast("Musisz uzupełnić login i hasło")
            return
        }

        di<Auth>().login(username, password, {
            toast("Zalogowano pomyślnie")
            // If login was successful, start the game & clear forms
            startActivity<GameActivity>()
            // Reset form
            this.username.setText("")
            this.password.setText("")
        },
        {
            toast("Nie można zalogować")
            this.password.setText("")
        })
    }
}
