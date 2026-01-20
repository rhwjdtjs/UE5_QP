<!-- ============================================================
 Project SZ (DEV) README
 - Branch: dev
 - Purpose: internal development / integration document
============================================================= -->

<p align="center">
  <img src="https://github.com/user-attachments/assets/c2ffa907-f8c1-40c4-9fea-7db016541f31" width="100%" alt="Project SZ Key Art"/>
</p>

<h1 align="center">Project SZ (DEV)</h1>

<p align="center">
  UE5(C++) 기반 <b>Listen Server Co-op 생존/탈출</b> 프로젝트의 <b>개발/통합 브랜치(dev) 전용 문서</b><br/>
  공개용 소개/로드맵은 <code>main https://github.com/rhwjdtjs/UE5_QP/blob/main/README.md  </code> 브랜치 <code>README.md</code>를 기준으로 합니다.
</p>
 
<p align="center">
  <img src="https://img.shields.io/badge/Engine-UE%205.5.4-black?style=for-the-badge" />
  <img src="https://img.shields.io/badge/Branch-dev-blue?style=for-the-badge" />
  <img src="https://img.shields.io/badge/Network-Listen%20Server-2ea44f?style=for-the-badge" />
  <img src="https://img.shields.io/badge/Language-C%2B%2B-orange?style=for-the-badge" />
</p>

---

## 목차
- [0) dev 브랜치 목적](#0-dev-브랜치-목적)
- [1) 개발 환경](#1-개발-환경)
- [2) 클론 & 브랜치 세팅 (복사용 코드)](#2-클론--브랜치-세팅-복사용-코드)
- [3) 실행/빌드](#3-실행빌드)
- [4) 협업 규칙 (Workflow)](#4-협업-규칙-workflow)
- [5) dev 머지 전 체크리스트](#5-dev-머지-전-체크리스트)
- [6) 저작권/라이선스 안전 규칙](#6-저작권라이선스-안전-규칙)
- [7) Git 규칙 (.gitignore 포함)](#7-git-규칙-gitignore-포함)
- [8) 코드 컨벤션 (권장)](#8-코드-컨벤션-권장)
- [9) 현재 개발 포커스 (dev 기준)](#9-현재-개발-포커스-dev-기준)
- [10) 자주 쓰는 Git 명령 모음](#10-자주-쓰는-git-명령-모음)

---

## 0) dev 브랜치 목적
- `dev`는 **기능 통합 + QA(PIE 멀티/네트워크 검증)** 용 브랜치입니다.
- 기능 개발은 `feature/*`에서 하고, PR로 `dev`에 합칩니다.
- `main`은 데모/릴리즈 가능한 **안정 상태**만 유지합니다.

---

## 1) 개발 환경
- **Unreal Engine**: `5.5.4`
- **OS**: Windows 10/11
- **IDE**: Visual Studio 2022 (Desktop development with C++)

---

## 2) 클론 & 브랜치 세팅 (복사용 코드)
> 아래 코드는 그대로 복사해서 Git Bash에서 실행하세요.

```bash
# 1) 레포 클론
git clone <REPO_URL>
cd UE5_QP

# 2) 원격 브랜치 최신화
git fetch origin

# 3) dev 브랜치 체크아웃 (원격 dev가 있으면 추적, 없으면 새로 생성)
git checkout dev 2>/dev/null || git checkout -b dev

# 4) dev를 원격에 푸시(처음 만들었을 때만)
git push -u origin dev
```

## 3) 실행/빌드

### 3.1 최초 1회
1. `.uproject` 우클릭 → **Generate Visual Studio project files**
2. Visual Studio에서 **Development Editor + Win64**로 빌드
3. UE Editor 실행

### 3.2 멀티플레이(PIE) 빠른 테스트
- Editor → Play → **Number of Players: 2**
- Net Mode: **Play as Listen Server**

✅ 최소 확인 항목
- 이동/점프/앉기 등 기본 입력
- 무기 장착/해제
- 공격(근접/총기)
- Replication/RPC 상태 동기화(서버 권한 판정)

---

## 4) 협업 규칙 (Workflow)

### 4.1 브랜치 전략
- `main`: 안정/배포(데모/릴리즈)
- `dev`: 통합/검증
- `feature/<topic>`: 기능 개발

### 4.2 feature 시작 규칙
```bash
git checkout dev
git pull origin dev

# 예시: feature/ai
git checkout -b feature/ai
```

### 4.3 작업 후 푸시
```bash
git status
git add .
git commit -m "feat: ai baseline"
git push -u origin feature/ai
```

### 4.4 PR 규칙

- PR 대상(base): dev
- PR 크기: 기능 단위
- 머지 방식: Squash merge

- ## 5) dev 머지 전 체크리스트
> PR 만들기 전에 아래 항목을 전부 확인합니다.

```text
[ ] 로컬 빌드 성공(Development Editor)
[ ] PIE 2인(리스닝 서버)에서 기능 정상 동작
[ ] 서버 권한(Authority) 기준으로 판정/상태가 맞는지 확인
[ ] 디버그 로그/라인 정리(필요 최소만)
[ ] .gitignore 대상 파일이 올라가지 않았는지 확인
[ ] 저작권/라이선스 문제 있는 파일 미포함
```

## 6) 저작권/라이선스 안전 규칙

- 출처/라이선스가 불명확한 애셋은 커밋 금지
- 타인 제작 애셋(애니/모델/사운드/텍스처/블루프린트 등)은 팀 내 사용 권한이 명확하거나(구매/배포권)
- 퍼블리셔 EULA/라이선스가 검증된 경우에만 포함
- 문제 발견 시: 즉시 브랜치/PR에서 제거, 필요하면 레포를 새로 파서 “완전 분리” 방식으로 정리

## 7) Git 규칙 (.gitignore 포함)

### 7.1 커밋 금지(대표)

- `Binaries/`
- `DerivedDataCache/`
- `Intermediate/`
- `Saved/`
- `.vs/`

### 7.2 .gitignore (복사용)

dev 브랜치에서 아래 내용을 그대로 복사해서 `.gitignore` 에 넣으세요.

# Unreal Engine
Binaries/
DerivedDataCache/
Intermediate/
Saved/
.vs/
.vscode/

# Visual Studio / Windows
*.VC.db
*.sln
*.suo
*.opensdf
*.sdf

# Build artifacts
Build/

## 8) 코드 컨벤션 (권장)

- UE 네이밍 규칙 준수(A/U/F/E/I prefix)
- 네트워크
- 서버 판정(Authority)에서 확정 → 결과만 복제
- RPC는 목적에 맞게(Server/Client/Multicast) 최소 사용

### 로그

- 카테고리 분리(DEFINE_LOG_CATEGORY)
- PR 전 과다 로그 제거

## 9) 현재 개발 포커스 (dev 기준)

- 전투/무기 코어 안정화 및 애님 상태 연동
- 멀티 환경에서 판정 불일치/지연 체감 줄이기 위한 디버깅 포인트 확보

## 10) 자주 쓰는 Git 명령 모음

# 브랜치 목록
git branch
git branch -r

# 변경 확인
git status
git diff

# 최신화
git fetch origin
git pull origin dev

# 브랜치 삭제(로컬)
git branch -d feature/ai

# 브랜치 삭제(원격)
git push origin --delete feature/ai

<p align="center"> <b>DEV 문서는 내부 개발/통합 목적이며, 공개용 설명은 <code>main README.md</code>를 기준으로 합니다.</b> </p> ```
