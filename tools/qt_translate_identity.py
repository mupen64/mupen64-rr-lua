#
#  Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
#
#  SPDX-License-Identifier: GPL-2.0-or-later
#

# Updates a Qt .ts file using the "source" text directly as the translation.

import sys
from pathlib import Path
from xml.dom import minidom

FILENAME = Path(__file__).name

COMMENT_TEXT = f"""
Auto-filled by {FILENAME}. DO NOT MODIFY.
"""


def insert_tag_comment(document: minidom.Document, root: minidom.Element):
    starting_ws = root.childNodes[0]
    comment_el = root.childNodes[1]

    # check if the tag is already there
    tag_present = (
        isinstance(starting_ws, minidom.Text)
        and starting_ws.isWhitespaceInElementContent
        and isinstance(comment_el, minidom.Comment)
        and comment_el.data == COMMENT_TEXT
    )
    if tag_present:
        return

    # insert comment and trailing whitespace
    insert_target = starting_ws.nextSibling
    assert not isinstance(insert_target, (
        minidom.ProcessingInstruction,
        minidom.DocumentType,
        minidom.Notation
    ))

    comment_el = minidom.Comment(data=COMMENT_TEXT)
    extra_ws = document.createTextNode("\n")
    if insert_target is not None:
        root.insertBefore(comment_el, insert_target)
        root.insertBefore(extra_ws, insert_target)
    else:
        root.appendChild(comment_el)
        root.appendChild(extra_ws)


def main():
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} [file]")
        sys.exit(1)

    document = minidom.parse(sys.argv[1])
    # check for the root element
    root = document.getElementsByTagName("TS")[0]

    # add tag comment
    insert_tag_comment(document, root)

    # iterate over the whole document
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
