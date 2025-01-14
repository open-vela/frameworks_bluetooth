/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***************************************************************************/

package com.openvela.bluetooth.adapter;

import java.util.List;
import android.content.Context;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

public abstract class RecyclerAdapter<T> extends RecyclerView.Adapter<RecyclerViewHolder> {
    protected Context mContext;
    protected int mLayoutId;
    protected List<T> mItems;

    public RecyclerAdapter(Context context, int layoutId, List<T> items) {
        mContext = context;
        mLayoutId = layoutId;
        mItems = items;
    }

    @NonNull
    @Override
    public RecyclerViewHolder onCreateViewHolder(@NonNull final ViewGroup parent, int viewType) {
        return RecyclerViewHolder.get(mContext, parent, mLayoutId);
    }

    @Override
    public void onBindViewHolder(@NonNull RecyclerViewHolder holder, int position) {
        onBindViewHolderItem(holder, mItems.get(position));
    }

    @Override
    public int getItemCount() {
        return mItems.size();
    }

    public abstract void onBindViewHolderItem(RecyclerViewHolder holder, T t);
}