package com.android.microdroid.prismapp;

import android.app.Fragment;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.content.Context;
import android.util.Log;

import java.lang.Long;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import java.io.File;
import java.io.FileOutputStream;

public class UnprotectedFormFragment extends Fragment {
	View view;
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
	  Bundle savedInstanceState) {
	  view = inflater.inflate(R.layout.fragment_unprotectedform, container, false);
	  final Button cancel = view.findViewById(R.id.button2);
          cancel.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
 
                ((MainActivity) getActivity()).showMain();
            }
          });	
		return view;
	}
}
