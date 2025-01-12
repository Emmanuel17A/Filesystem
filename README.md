# Filesystem
This is a file system management in C programming language, with gtk as the supporting UI
For you to run it you need to install gtk, to support the UI and the backend code, the actual filesystem
When committing warnings indicate that Git has detected a mismatch in the line-ending styles between the files in my repository and the system settings configured for Git.
The repository's files currently use LF line endings, but Git is configured to convert them to CRLF when these files are checked out or modified in the working directory (e.g., on a Windows system).
The warning is informing you that these files (Filesystem.cbp, Filesystem.cscope_file_list, and Filesystem.layout) will have their line endings changed from LF to CRLF the next time Git processes them (e.g., when checking out, committing, or switching branches).
Git has a configuration setting, core.autocrlf, that controls how it handles line endings:
true: Converts LF to CRLF on checkout and CRLF back to LF on commit.
false: No automatic conversion.
input: Converts CRLF to LF on commit, but doesn't modify line endings on checkout.
On Windows, core.autocrlf is often set to true by default, which triggers these conversions.
If you're on Windows and want to keep CRLF locally, use: git config --global core.autocrlf true
