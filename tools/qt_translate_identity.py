#
#  Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
#
#  SPDX-License-Identifier: GPL-2.0-or-later
#

# Updates a Qt .ts file using the "source" text directly as the translation.

import sys
from xml.dom import minidom


def main():
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} [file]")
        sys.exit(1)

    document = minidom.parse(sys.argv[1])

    # check for the root element
    root = document.childNodes[1]
    assert isinstance(root, minidom.Element) and root.tagName == "TS"


    for context in root.getElementsByTagName("context"):
        for message in context.getElementsByTagName("message"):
            source_el = message.getElementsByTagName("source")[0]
            translation_el = message.getElementsByTagName("translation")[0]

            while translation_el.firstChild is not None:
                translation_el.removeChild(translation_el.firstChild)

            source_text = source_el.childNodes[0]
            assert isinstance(source_text, minidom.Text)

            source_text_2 = source_text.cloneNode(True)
            assert source_text_2 is not None

            if translation_el.hasAttribute("type"):
                translation_el.removeAttribute("type")
            translation_el.appendChild(source_text_2)

    with open(sys.argv[1], "wb") as file:
        file.write(document.toxml(encoding="utf-8"))


if __name__ == "__main__":
    main()
