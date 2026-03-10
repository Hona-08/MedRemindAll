# MedRemind — Medication Reminder SaaS (Drupal 7)

A complete medication reminder and tracking application built as a custom Drupal 7 module, demonstrating both **Custom Tables** and **Entity API** approaches.

---

## Features

- **Medication CRUD** — Add, edit, delete medications with dosage, frequency, and multi-time scheduling
- **AJAX Dashboard** — Take/Skip doses without page reload with real-time stat updates
- **Streak Tracking** — Consecutive day tracking with milestone notifications (7, 14, 30, 60, 90, 365 days)
- **Toast Notifications** — Popup alerts with sound on Take/Skip actions
- **On-Site Notifications** — Bell icon with unread badge, dropdown preview, full notifications page
- **Cron Background Tasks** — Auto-missed dose marking, log cleanup, notification generation
- **Reports & Analytics** — Adherence charts, per-medication breakdown, streak leaderboard
- **CSV Export/Import** — Export medications/history/full data, import with validation
- **Prescription Entity** — Entity API-based prescription tracking with refills and expiry
- **User System** — Custom branded login, registration, profile with stats, logout
- **Admin Settings** — Global configuration with live system statistics
- **Pagination** — Paged history with Drupal's pager system
- **37 Automated Tests** — SimpleTest covering installation, access, CRUD, AJAX, admin, entity
- **Responsive Design** — Desktop, tablet, mobile with CSS Grid and Flexbox

---

## Technical Overview

| Metric | Count |
|--------|-------|
| Database Tables | 8 (7 custom + 1 entity) |
| Hooks Implemented | 13 |
| Routes/Endpoints | 24 |
| Forms | 10 |
| Include Files | 12 |
| Template Files | 3 |
| Test Cases | 37 |
| Cron Tasks | 7 |
| CSS Lines | 1250+ |

---

## Database Tables

| # | Table | Type | Purpose |
|---|-------|------|---------|
| 1 | `medremind_medications` | Custom | Medication data |
| 2 | `medremind_schedule` | Custom | Dose times |
| 3 | `medremind_log` | Custom | Action history |
| 4 | `medremind_reminders` | Custom | Reminder queue |
| 5 | `medremind_streaks` | Custom | Streak tracking |
| 6 | `medremind_settings` | Custom | User preferences |
| 7 | `medremind_notifications` | Custom | On-site notifications |
| 8 | `medremind_prescriptions` | Entity | Prescriptions (Entity API) |

---

## Hooks Implemented (13)

hook_init, hook_menu, hook_permission, hook_theme, hook_schema, hook_install, hook_uninstall, hook_cron, hook_mail, hook_mail_alter, hook_block_info, hook_block_view, hook_entity_info

---

## File Structure

```
medremind/
├── medremind.info
├── medremind.module              (13 hooks + NullMailSystem class)
├── medremind.install             (8 tables + 3 update hooks)
├── medremind.test                (37 tests, 7 classes)
├── medremind.css                 (1250+ lines)
├── js/medremind.js               (AJAX + Toast + Web Audio)
├── includes/
│   ├── medremind.pages.inc       (Dashboard + History with pagination)
│   ├── medremind.forms.inc       (Add/Edit/Delete + User settings)
│   ├── medremind.admin.inc       (Admin settings)
│   ├── medremind.ajax.inc        (Take/Skip + streak logic)
│   ├── medremind.blocks.inc      (Sidebar blocks + notification bell)
│   ├── medremind.cron.inc        (7 cron tasks)
│   ├── medremind.notifications.inc (CRUD + pages + AJAX)
│   ├── medremind.reports.inc     (Charts + breakdown + streaks)
│   ├── medremind.export.inc      (CSV export/import)
│   ├── medremind.entity.inc      (Prescription Entity classes)
│   ├── medremind.prescription.pages.inc (Prescription forms)
│   └── medremind.user.inc        (Login/Register/Profile/Logout)
└── templates/
    ├── medremind-dashboard.tpl.php
    ├── medremind-med-card.tpl.php
    └── medremind-history.tpl.php
```

---

## Custom Tables vs Entity API

| Feature | Custom Tables | Entity API |
|---------|--------------|------------|
| Used for | Medications, Log, Streaks | Prescriptions |
| CRUD | db_insert/update/delete | entity_save/load/delete |
| Forms | Manual Form API | Auto-generated |
| Fieldable | No | Yes |
| Views | Manual queries | Automatic |

---

## Security

check_plain (XSS), parameterized queries (SQL injection), form_set_error (validation), access callbacks (authorization), ownership checks (uid match), user_authenticate (login), valid_email_address, password min length, username restrictions

---

## Requirements

- Drupal 7.x, PHP 5.6+, MySQL 5.5+, Entity API module

---

## Installation

1. Enable Entity API module
2. Copy medremind/ to sites/all/modules/custom/
3. Enable MedRemind at Admin → Modules
4. Grant "Use MedRemind" to authenticated user role
5. Configure at Admin → Configuration → MedRemind Settings

---

## Testing

```
Admin → Modules → Enable "Testing"
Admin → Configuration → Development → Testing → Check "MedRemind" → Run
```

---

## Author

Built by **Hona** — 8 tables, 13 hooks, Entity API, Form API, AJAX, toast notifications, cron automation, 37 tests, responsive CSS.

## License