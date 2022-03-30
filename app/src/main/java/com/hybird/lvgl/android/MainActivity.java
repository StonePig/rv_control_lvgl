package com.hybird.lvgl.android;

import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;

import com.hybird.lvgl.android.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {


    private ActivityMainBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());
    }
}