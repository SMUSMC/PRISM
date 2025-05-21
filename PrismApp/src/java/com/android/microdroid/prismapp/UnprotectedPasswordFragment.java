package com.android.microdroid.prismapp;

import android.app.Fragment;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.content.Context;
import android.util.Log;
import android.widget.Toast;
import android.widget.EditText;

import java.lang.Long;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import java.io.File;
import java.io.FileOutputStream;

public class UnprotectedPasswordFragment extends Fragment {
	View view;
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
	  Bundle savedInstanceState) {
	  view = inflater.inflate(R.layout.fragment_unprotectedpasswordform, container, false);
	  final Button cancel = view.findViewById(R.id.button2);
          cancel.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                
                ((MainActivity) getActivity()).showMain();
            }
          });	  
          final Button submit = view.findViewById(R.id.button);
          submit.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                 EditText pwdtext;
                 pwdtext = (EditText)view.findViewById(R.id.editText);
                 String s1 = pwdtext.getText().toString();
                 if (s1.equals("password1"))
                   ((MainActivity) getActivity()).showUnprotected();
                 else{  
                   Log.e("i","unprotected. wrong password.");
                   Context context = ((MainActivity) getActivity()).getApplicationContext();
                   Toast toast = Toast.makeText(context,"Wrong password",Toast.LENGTH_SHORT);
                 }
                   
          }  
          });	
		return view;
	}
}
