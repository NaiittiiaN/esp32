# Active Context: Next.js Starter Template

## Current State

**Template Status**: ✅ Extended with embedded firmware assets and ESP-IDF firmware workspace

The repository now contains the original Next.js starter plus two ESP32 firmware workspaces: an existing Arduino/Matter sketch and a new ESP-IDF-native project for sensor development, diagnostics, and future local ESP-Matter integration.

## Recently Completed

- [x] Base Next.js 16 setup with App Router
- [x] TypeScript configuration with strict mode
- [x] Tailwind CSS 4 integration
- [x] ESLint configuration
- [x] Memory bank documentation
- [x] Recipe system for common features
- [x] Added ESP32 Matter firmware for AM2320 and DS18B20 sensors
- [x] Saved required Arduino sensor libraries locally in `third_party/`
- [x] Added ESP-IDF firmware project for ESP32-WROOM DevKitV1 with local AM2320 and DS18B20 drivers
- [x] Added local diagnostics/calibration web UI with JSON API and NVS-backed calibration offsets
- [x] Added Wi-Fi and Matter state logging with stub-compatible Matter bridge for offline local component vendoring

## Current Structure

| File/Directory | Purpose | Status |
|----------------|---------|--------|
| `src/app/page.tsx` | Home page | ✅ Ready |
| `src/app/layout.tsx` | Root layout | ✅ Ready |
| `src/app/globals.css` | Global styles | ✅ Ready |
| `.kilocode/` | AI context & recipes | ✅ Ready |
| `firmware/esp32-matter-sensors/` | ESP32 Matter sketch and setup guide | ✅ Ready |
| `firmware/esp32-idf-matter-sensors/` | ESP-IDF-native firmware with local drivers, web UI, and Matter bridge stub | ✅ Ready |
| `third_party/` | Vendored Arduino libraries for firmware build | ✅ Ready |

## Current Focus

Current focus areas in the repo:

1. Next.js application work when requested
2. ESP32 firmware iteration for Matter-based sensors on Arduino and ESP-IDF stacks
3. Keeping local library dependencies and drivers versioned inside the project
4. Preparing local ESP-Matter vendoring path for full Matter commissioning support

## Quick Start Guide

### To add a new page:

Create a file at `src/app/[route]/page.tsx`:
```tsx
export default function NewPage() {
  return <div>New page content</div>;
}
```

### To add components:

Create `src/components/` directory and add components:
```tsx
// src/components/ui/Button.tsx
export function Button({ children }: { children: React.ReactNode }) {
  return <button className="px-4 py-2 bg-blue-600 text-white rounded">{children}</button>;
}
```

### To add a database:

Follow `.kilocode/recipes/add-database.md`

### To add API routes:

Create `src/app/api/[route]/route.ts`:
```tsx
import { NextResponse } from "next/server";

export async function GET() {
  return NextResponse.json({ message: "Hello" });
}
```

## Available Recipes

| Recipe | File | Use Case |
|--------|------|----------|
| Add Database | `.kilocode/recipes/add-database.md` | Data persistence with Drizzle + SQLite |

## Pending Improvements

- [ ] Add more recipes (auth, email, etc.)
- [ ] Add example components
- [ ] Add testing setup recipe

## Session History

| Date | Changes |
|------|---------|
| Initial | Template created with base setup |
| 2026-04-15 | Added ESP32 Matter firmware for AM2320 + DS18B20 with dual Wi-Fi credentials and local vendored Arduino libraries |
| 2026-04-15 | Added `firmware/esp32-idf-matter-sensors/` with ESP-IDF-native sensor drivers, Wi-Fi fallback manager, web diagnostics/calibration UI, log ring buffer, and conditional Matter bridge stub awaiting local `esp_matter` sources |
