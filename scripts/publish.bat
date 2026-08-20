rem 本地手动发布到 pub.dev
rem 发布前先执行 dry-run 检查，确认无误后去掉注释再正式发布
rem flutter packages pub publish --dry-run
flutter packages pub publish --server=https://pub.dartlang.org
