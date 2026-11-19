# Team 3 Development Process
## Embedded Project Compatibility and Price Checker

## 1. Repository Architecture

### 1.1 Repository structure

```text
EmbeddedMarket/
├── .gitignore
├── README.md
├── docker-compose.yml
│
├── backend/
│   ├── app.py
│   ├── compatibility/
│   │   ├── __init__.py
│   │   ├── checker.py
│   │   ├── handler.py
│   │   └── store.py
│   ├── pricing/
│   │   ├── __init__.py
│   │   ├── catalog.py
│   │   └── handler.py
│   ├── tests/
│   │   ├── test_api.py
│   │   └── test_check.py
│   ├── .dockerignore
│   ├── Dockerfile
│   └── requirements.txt
│
├── frontend/
│   ├── src/
│   │   ├── App.jsx
│   │   ├── Screens.jsx
│   │   ├── api.js
│   │   ├── data.js
│   │   ├── main.jsx
│   │   └── style.css
│   ├── .dockerignore
│   ├── Dockerfile
│   ├── index.html
│   ├── nginx.conf
│   ├── package.json
│   └── package-lock.json
│
├── engine/
│   ├── include/embedded_market/
│   │   ├── engine/
│   │   ├── model/
│   │   ├── rules/
│   │   └── util/
│   ├── src/
│   │   ├── engine/
│   │   ├── model/
│   │   ├── rules/
│   │   └── util/
│   ├── service/
│   │   ├── CMakeLists.txt
│   │   ├── JsonCodec.cpp
│   │   ├── JsonCodec.hpp
│   │   └── main.cpp
│   ├── tests/
│   ├── .dockerignore
│   ├── CMakeLists.txt
│   ├── Dockerfile
│   └── README.md
│
├── database/
│   └── init/
│       ├── 01_schema.sql
│       └── 02_seed.sql
│
└── doc/
    ├── dev_process.md
    └── Semester_1/
        └── SDP.pdf
```

### 1.2 Directory descriptions

Each service gets its own directory at the root level of the repo. Documentation also gets its own directory. The only other things at the root level are required to be, like `docker-compose.yml` and `README.md` and `.gitignore`. `backend/` holds the Flask backend. Each feature gets a folder, allowing for modularity. `backend/app.py` connects all the routes for each feature. `frontend/` handles the UI, calls to the backend, styling, and displaying received data. The main code is stored in `frontend/src/`. The C++ engine is in `engine/`, and stores the implementations in `engine/src/`. Headers stay in the `engine/include/`, and API interfacing is handled with `engine/service/`. Lastly, `database/` stores the initial migrations for the database schema, as needed for this prototype mockup.

### 1.3 References

[Monorepo Project Structure](https://www.stacklesson.com/mean-stack-tutorial/mean-intro/project-structure-monorepo/)

## 2. Branching and Workflow Model

### 2.1 Branches and their purpose

Our team focuses on using a feature branch approach for the workflow. We have one long lived branch, main, which is used to keep the integrated version of the project and is used for testing and demonstrations. Each teammate creates and maintains their own shorter lived branched for each of their tasks in the shared team repo. We do not use personal forks.

### 2.2 Branch naming convention

We use feature/<task-name>-<initials> for the new functionalities and fix/<task-name> for anything regarding bugs in the code. Tasks and bugs use lowercase worlds that are separated by hyphens and describes the feature/fix. For example, feature/component-selection adds component-selection interface, while fix/empty-selection works on correcting an issue with leaving a section blank.

### 2.3 Development workflow

At the start of the task, the teammate developing pulls the latest version of the main and creates a branch, naming it using the format above. They commit and push their changes to their branch. Once that task is read, they open a pull request into main and then loop in another teammate to review the policy. After the pull request is merged, teammates can pull the updated main and work on further developing or testing it all together. 

## 3. Code Development and Review Policy

### 3.1 Pull requests

Each teammate opens a pull request into main when their task is ready for review. Keep changes focused on one task and include a short description, how the changes were tested, and any known issues. Include screenshots for UI changes.

### 3.2 Reviews and approvals

One teammate other than the author must review and approve each pull request. We aim to review requests within 24 hours and communicate any delays. Reviewers check that the code is understandable, works as described, and fits with the rest of the project. Required fixes must be resolved before merging.

### 3.3 Testing and automated checks

Before merging, the affected code must build or start successfully, and the author must manually test the changed functionality. Changes involving multiple services must be tested together. Any existing automated tests and configured GitHub Actions checks must pass. We will set up a basic automated build or syntax check for pull requests. Before the demo, the team will test the full user flow, including at least one working API call.

### 3.4 Merging

The author can merge their pull request after receiving approval, passing required checks, and resolving review comments. Changes must go through pull requests instead of being pushed directly to main. After merging, delete the completed branch and confirm the updated project still works. Merge completed tasks regularly to catch integration problems early.
