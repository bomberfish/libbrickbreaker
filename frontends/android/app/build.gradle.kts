plugins {
    alias(libs.plugins.android.application)
}

android {
    namespace = "ca.bomberfish.brickbreaker"
    compileSdk {
        version = release(36) {
            minorApiLevel = 1
        }
    }

    // Share game assets with the SDL / Emscripten frontends from the top-level
    // assets/ directory rather than keeping a duplicate copy under
    // frontends/android/app/src/main/assets/.
    sourceSets["main"].assets.srcDirs("${rootDir}/../../assets")

    defaultConfig {
        applicationId = "ca.bomberfish.brickbreaker"
        minSdk = 21
        targetSdk = 36
        versionCode = 1
        versionName = "1.0"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
    buildFeatures {
        prefab = true
    }
    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
}

dependencies {
    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.appcompat)
    implementation(libs.material)
    implementation(libs.androidx.games.activity)
    testImplementation(libs.junit)
    androidTestImplementation(libs.androidx.junit)
    androidTestImplementation(libs.androidx.espresso.core)
}
