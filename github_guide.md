# Let-us-Connect-HTTP-Server



## Team Git Workflow Guide

Welcome to the project! This is a quick and small guide to get you to know the commands that will mostly be used while developing the project.

These commands will be run from the terminal, however, if you are using GUI (ex. vscode gui) then you can just use the GUI options with the commands names given below with the exception (probably) of the configuration part.

**Disclaimer: AI is used in making this guide so maybe there will be some mistakes**

### ⚙️ Prerequisites: One-Time Git Setup
Before you write any code, you need to link your local Git to your GitHub account. Run these commands in your terminal **(Note: only use the --global option if you want to set these configurations for the entire computer, otherwise don't write)**:

```bash
# Set your name (this will appear on your commits)
git config --global user.name "Your Name"

# Set your email (MUST match your GitHub account email exactly)
git config --global user.email "your.email@example.com"

# Set the default branch name to 'main' instead of 'master'
git config --global init.defaultBranch main
```
*(Note: When you push code for the first time, Git will ask you to log in. Use the browser popup if it appears, or generate a Personal Access Token on GitHub if you are asked for a password in the terminal.)*

---

###🚀 The Daily Workflow

#### Phase 1: Starting Your Work
Always make sure you are creating a new branch for your specific task. **Never work directly on the `main` branch.**

```bash
# 1. Download the repository (only needed the very first time)
git clone https://github.com/George2251/Let-us-Connect-HTTP-Server.git
cd <repository_folder_name>

# 2. Create a new branch for your feature and switch to it immediately
# Give it a descriptive name (e.g., git checkout -b add-http-parser)
git checkout -b <feature_branch_name>
```

#### Phase 2: Writing Code & Saving Progress
As you write your code, bundle your changes into commits and send them to GitHub so they don't get lost.

```bash
# 1. Stage the specific files you worked on
git add <file1> <file2> 

# 2. Package those changes with a clear, short message
git commit -m "Add error handling for 404 responses"

# 3. Send your branch to GitHub. 
# IMPORTANT: The VERY FIRST time you push a new branch, use this exact command:
git push -u origin <feature_branch_name>

# For any future pushes on this same branch, you can just type:
git push
```

#### Phase 3: Merging Your Code (The Rebase Flow)
Before opening a Pull Request, ensure your code is compatible with any updates merged into `main` while you were working.

```bash
# 1. Download the latest hidden updates from GitHub without changing files yet
git fetch origin

# 2. Re-apply your feature branch commits on top of the most recent main branch
git rebase origin/main
```

**⚠️ If you get a CONFLICT during the rebase:**
Git will pause. Open the conflicted files in your IDE, look for the conflict markers (`<<<<<<<`), and manually fix the code to make it work. (this maybe facilitated by your IDE/code editor.)

```bash
# 3. Tell Git you have resolved the conflict in those files
git add <resolved_file1> <resolved_file2>

# 4. Continue the rebase process (DO NOT use 'git commit' here!)
git rebase --continue
```

**After the rebase is completely finished:**

```bash
# 5. Force-push your freshly rebased branch to GitHub. 
# Use --force-with-lease as a safety net to avoid overwriting others' work.
git push --force-with-lease 
```

#### Phase 4: The Pull Request
1. Go to the repository page on GitHub.
2. Click the green **"Compare & pull request"** button that appears.
3. Add a brief description of what your code does.
4. Submit it for the Admin to review!