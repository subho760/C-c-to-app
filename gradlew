#!/usr/bin/env bash

# Help Message
if [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
    echo "Usage: gradlew [task...]"
    exit 0
fi

# Find the project root
DIRNAME="$(dirname "$0")"
[ -z "$DIRNAME" ] && DIRNAME="."
APP_BASE_NAME="$(basename "$0")"
APP_HOME="$(cd "$DIRNAME" && pwd)"

# Setup Java Executable
if [ -n "$JAVA_HOME" ] ; then
    JAVACMD="$JAVA_HOME/bin/java"
else
    JAVACMD="java"
fi

CLASSPATH=$APP_HOME/gradle/wrapper/gradle-wrapper.jar

# Execute Gradle
exec "$JAVACMD" "-Dorg.gradle.appname=$APP_BASE_NAME" -classpath "$CLASSPATH" org.gradle.wrapper.GradleWrapperMain "$@"
