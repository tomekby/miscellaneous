package pl.vot.tomekby.mathGame

import android.view.View
import android.view.ViewGroup
import android.widget.BaseAdapter
import android.widget.Button
import android.widget.LinearLayout
import org.jetbrains.anko.*
import org.jetbrains.anko.sdk25.coroutines.onClick

class GameResultsAdapter(
    private val items: ArrayList<Long> = ArrayList(),
    private val handler: (Long, View?) -> Unit
) : BaseAdapter() {
    private val itemsPerRow = 2
    private val buttons = HashMap<Long, Button>()

    override fun getView(position: Int, convertView: View?, parent: ViewGroup?): View {
        return with(parent!!.context) {
            linearLayout {
                orientation = LinearLayout.HORIZONTAL
                // if itemsPerRow = 3 then best works ~30
                horizontalPadding = dip(60)
                verticalPadding = dip(10)

                getItem(position).forEach { result ->
                    buttons[result] = button(result.toString()) {
                        // Set on click handler for each created button
                        onClick  { view -> handler(result, view) }
                    }.lparams(width = wrapContent) {
                        verticalMargin = dip(5)
                        horizontalMargin = dip(15)
                    }
                }
            }
        }
    }

    // Get values for selected row
    override fun getItem(position: Int): List<Long> {
        val rowStart = position * itemsPerRow
        return items.slice(
            rowStart..(rowStart + itemsPerRow - 1)
        )
    }

    override fun getItemId(position: Int): Long {
        return 0L
    }

    override fun getCount(): Int {
        return items.size / itemsPerRow
    }

    fun disableButtons() {
        buttons.forEach { _, b -> b.isClickable = false }
    }
}