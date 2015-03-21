/*
 *  TreeViewWidget.hpp
 *
 *  Created by freanux on Mar 18, 2015
 *  Copyright 2015 Circada Team. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _TREEVIEWWIDGET_HPP_
#define _TREEVIEWWIDGET_HPP_

#include "Terminal.hpp"
#include "ScreenWindow.hpp"

#include <vector>
#include <string>

struct TreeViewEntry {
    TreeViewEntry(ScreenWindow *win, bool coloring, const std::string& sep, const std::string& entry);
    ScreenWindow *win;
    bool coloring;
    std::string sep;
    std::string entry;
};

class TreeViewWidget {
public:
    typedef std::vector<TreeViewEntry> TreeView;
    TreeViewWidget();
    virtual ~TreeViewWidget();

    void configure(int height, int width, int posy, int posx);
    int get_estimated_width();
    TreeView& get_treeview();
    void draw(ScreenWindow *selected_window);

private:
    bool configured;
    int height;
    int width;
    WINDOW *win_treeview;
    std::string separator;
    TreeView treeview;

    void delete_ncurses_object();
};

#endif // _TREEVIEWWIDGET_HPP_
