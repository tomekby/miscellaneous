package pl.vot.tomekby.mathGame

import android.annotation.SuppressLint
import android.graphics.Typeface.DEFAULT_BOLD
import android.view.View
import android.view.ViewGroup
import android.widget.BaseAdapter
import android.widget.LinearLayout.HORIZONTAL
import org.jetbrains.anko.*
import pl.vot.tomekby.mathGame.domain.HighScoreDTO
import kotlin.collections.ArrayList

class HighScoreAdapter(private val items: ArrayList<HighScoreDTO> = ArrayList()) : BaseAdapter() {
    @SuppressLint("SetTextI18n", "SimpleDateFormat")
    override fun getView(position: Int, convertView: View?, parent: ViewGroup?): View {
        return with(parent!!.context) {
            linearLayout {
                id = R.id.listItemContainer
                lparams(width = matchParent, height = wrapContent)
                orientation = HORIZONTAL
                padding = dip(10)

                val result = items[position]
                textView {
                    text = result.member
                    typeface = DEFAULT_BOLD
                    padding = dip(5)
                    width = 250
                }
                textView {
                    text = "%.3f s.".format(result.time)
                    padding = dip(5)
                    width = 250
                }
                textView {
                    text = "Data: ${result.dateFinished}"
                    padding = dip(5)
                }
            }
        }
    }

    override fun getItem(position: Int): HighScoreDTO {
        return items[position]
    }

    override fun getItemId(position: Int): Long {
        return 0L
    }

    override fun getCount(): Int {
        return items.size
    }

    fun add(newItems: List<HighScoreDTO>) {
        items.addAll(newItems)
        notifyDataSetChanged()
    }
}