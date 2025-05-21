package com.android.microdroid.prismapp;

import android.app.Fragment;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import android.view.MotionEvent;
import android.util.Log;
import android.content.Context;

public class PasswordFragment extends Fragment {

	View view;

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
		Bundle savedInstanceState) {
		view = inflater.inflate(R.layout.fragment_key, container, false);


		view.setOnTouchListener(new View.OnTouchListener() {
                   public boolean onTouch(View v, MotionEvent event) {
                      if(event.getAction() == MotionEvent.ACTION_DOWN){        

                        float x = event.getX();
                        float y = event.getY();
                        //Log.i("i","Password Clicked x: "+x+" y: "+y+"\n");
                        if (y>1500){
                           //Log.i("i","Checking Password");
		            if(((MainActivity) getActivity()).getModel().checkPassword()==1){
		              //Log.i("i","Password Correct");
		              ((MainActivity) getActivity()).getModel().protectedFormCreation();
                 
		              ((MainActivity) getActivity()).showProtected();             
		            }
		        }
		        else{
		            ((MainActivity) getActivity()).getModel().protectedKeyInput((int)x, (int)y);
		            TextView tv = view.findViewById(R.id.textView3);
		            String textstring = Float.toString(x) ;
		            textstring = textstring + "," + Float.toString(y);
		            tv.setText(textstring);
		        }
                      }
                      return true;
                   }
                });
		return view;
	}
}
