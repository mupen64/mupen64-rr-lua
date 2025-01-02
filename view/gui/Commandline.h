/***************************************************************************
                                                  commandline.h  -  description
                                                         -------------------
        copyright            : (C) 2003 by ShadowPrince
        email                : shadow@emulation64.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef COMMANDLINE_H
#define COMMANDLINE_H

namespace Cli
{
    /**
     * \brief Initializes the CLI
     */
    void init();

    /**
     * Gets whether the CLI wants fast-forward to always be enabled.
     */
    bool wants_fast_forward();
} // namespace Cli
#endif
