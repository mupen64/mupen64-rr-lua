# pip install PyGithub
# pip install pick

from github import Github
from github import Auth
from pick import pick

START_COMMIT = "YOUR_COMMIT_SHA_HERE"
GITHUB_TOKEN = "YOUR_TOKEN_HERE"

auth = Auth.Token(GITHUB_TOKEN)
g = Github(auth=auth)

repo = g.get_repo("mupen64/mupen64-rr-lua")

inital_commit = repo.get_commit(START_COMMIT)
initial_commit_date = inital_commit.commit.author.date

commits = repo.get_commits(since = initial_commit_date)

features: list[str] = []
changes: list[str] = []
bugfixes: list[str] = []
dev: list[str] = []

def prompt_for_commit_kind(text):
    title = f"'{text}' - intent couldn't be detected automatically. Please choose the commit type: "
    options = ['Feat', 'Chg', 'Fix', 'Dev', 'Discard']
    option, _ = pick(options, title)
    return option

for commit in commits:
    msg = commit.commit.message

    msg = msg.split("\n")[0]
    kind = ""
    module = ""
    message = ""
    module_with_message = ""

    if msg.find(":") != -1:
        string_up_to_separator = msg[:msg.find(":")]

        parts = string_up_to_separator.split("/")

        module_with_message = f"{string_up_to_separator}{msg[msg.find(":"):]}"

        if len(parts) == 1:
            kind = prompt_for_commit_kind(msg)
        else:
            if parts[-1].casefold() in { "feat", "chg", "fix", "dev" }:
                module_with_message = f"{string_up_to_separator[:string_up_to_separator.rfind("/")]}{msg[msg.find(":"):]}"
                kind = parts[-1].casefold()
            else:
                kind = prompt_for_commit_kind(msg)
    else:
        print(f"'{msg}' ignored due to no colon")

    match kind.casefold():
        case "feat":
            features.append(module_with_message)
        case "chg":
            changes.append(module_with_message)
        case "fix":
            bugfixes.append(module_with_message)
        case "dev":
            dev.append(module_with_message)
        case _:
            print(f"'{msg} discarded")

with open('changelog.md', 'w') as f:

    def write_section(title, values: list[str]):
        if len(values) == 0:
            return
        
        f.write(f'# {title}\n')
        for str in sorted(values):
            f.write(f"- {str}\n")
        f.write("\n")
        
    write_section("Features", features)
    write_section("Changes", changes)
    write_section("Bugfixes", bugfixes)
    write_section("Dev", dev)

g.close()
